#include "stdafx.h"
#include "gapi.h"
#include "util.h"

#include <KR3/util/process.h>
#include <KR3/data/crypt.h>
#include <KRUtil/text/slash.h>
#include <KRUtil/parser/jsonparser.h>
#include <KRUtil/fs/file.h>
#include <KRUtil/net/socket.h>
#include <KRUtil/fetch.h>
#include <KRMessage/pool.h>
#include <KRMessage/eventdispatcher.h>
#include <KRHttp/httpd.h>
#include <KR3/data/crypt.h>

#define USER_DATA_URL	"res/loginpage/.vscode/data"
#define SCOPES			"https://www.googleapis.com/auth/youtube%20https://www.googleapis.com/auth/cloud-platform"
#define GOOGLE_ID		"957160054090-v6h28n1abhr6c45jdqrlbu6igre66vid.apps.googleusercontent.com"
#define GOOGLE_SECRET	"DLcLAOmrYpy5MNzQ2o1nMOwP"
#define TWITCH_ID		"bmu3rpxcx6sdew8dr31ntcsugb11uu"
#define TWITCH_SECRET	"hxm2t7n4mtd9j949ba9qt7njrg0zi6"
#define GOOGLE_REFRESH_TOKEN_FILE u"temp/google_refresh_token"
#define TWITCH_ACCESS_TOKEN_FILE u"temp/twitch_access_token"
#define GOOGLE_COOKIE_FILE u"temp/twitch_access_token"

using namespace kr;
namespace
{
	AText googleAccessToken;
	AText googleRefreshToken;
	AText googleYouTubeCookie;
	AText twitchAccessToken;
	AText twitchIdToken;

	const char16 BADGE_INFO_JSON[] = u"temp/badges.json";

	class TwitchBadgeImage
	{
	private:
		Promise<void> * m_loading;
		Map<Text, Map<int, AText>> m_map;
		
	public:
		TwitchBadgeImage() noexcept
		{
			m_loading = nullptr;
		}
		~TwitchBadgeImage() noexcept
		{
		}

		Promise<void> * load() noexcept
		{
			if (m_loading)
			{
				return m_loading;
			}

			return m_loading = threading([this] {
				File * file;
				try
				{
					file = File::open(BADGE_INFO_JSON);
					goto _loadContinue;
				}
				catch (...)
				{
				}
				try
				{
					HttpRequest request;
					request.fetchAsFileSync("https://badges.twitch.tv/v1/badges/global/display", BADGE_INFO_JSON);
					file = File::open(BADGE_INFO_JSON);
					goto _loadContinue;
				}
				catch (...)
				{
					return;
				}
			_loadContinue:
				io::FIStream<char> fis = file;
				JsonParser parser(&fis);
				parser.object([&](Text name){
					if (name == "badge_sets")
					{
						parser.object([&](Text name){
							auto & versions = m_map[name];
							parser.object([&](Text versionsText) {
								if (versionsText == "versions")
								{
									parser.object([&](Text version) {
										AText &urlDest = versions[version.to_uint()];
										parser.object([&](Text name) {
											if (name == "image_url_1x")
											{
												urlDest = parser.text();
											}
											else
											{
												parser.skipValue();
											}
										});
									});
								}
								else
								{
									parser.skipValue();
								}
							});
						});
					}
					else
					{
						parser.skipValue();
					}
				});
			})->then([this] {
				m_loading = nullptr;
			});
		}

		Text getUrl(Text name, uint version) noexcept
		{
			Text url;
			if (!m_map.ifGet(name, [&](const Map<int, AText>& versions) {
				auto iter = versions.find(version);
				if (iter == versions.end())
				{
					url = nullptr;
				}
				else
				{
					url = iter->second;
				}
			})) return nullptr;
			return url;
		}
	};

	TwitchBadgeImage s_badges;

	void reformBase64(AText & buffer)
	{
		buffer.change('+', '-');
		buffer.change('/', '_');

		Text end = buffer.find_nr('=');
		if (end != nullptr)
		{
			buffer.cut_self(end + 1);
		}
	}
	AText randomBase64(size_t bytes) noexcept
	{
		TmpArray<char> data;
		data.resize(bytes);
		g_random.fill(data.data(), bytes);

		AText text;
		text << (encoder::Base64)data;
		reformBase64(text);
		return text;
	}
	int parseToken(Text response) noexcept
	{
		AText refreshToken;
		int expiresIn = -1;
		JsonParser(&response).fields([&](JsonField & field) {
			field("access_token", &googleAccessToken);
			field("expires_in", &expiresIn);
			field("refresh_token", &refreshToken);
		});
		if (refreshToken != nullptr)
		{
			googleRefreshToken = move(refreshToken);
			File::saveFromArray<char>(GOOGLE_REFRESH_TOKEN_FILE, (Text)googleRefreshToken);
		}
		return expiresIn;
	}
}

class HttpReceiver
{
private:
	Socket * m_socket;
	Socket * m_client;
	uint m_port;
	io::SelfBufferedIStream<io::SocketStream<char>> m_is;
	AText m_data;

	uint _openRandomPort() noexcept
	{
		const uint PORT_START = 10000;
		const uint PORT_RANGE = 10000;
		uint port_start = PORT_START + (g_random.get<dword>() % PORT_RANGE);
		uint port = port_start;
		for (;;)
		{
			try
			{
				m_socket->open(port, Ipv4Address::loopBack);
				return port;
			}
			catch (SocketException&)
			{
			}
			port++;
			if (port >= PORT_START + PORT_RANGE) port = PORT_START;
			if (port == port_start) return 0;
		}
	}

public:
	HttpReceiver() noexcept
		:m_is(nullptr)
	{
		m_socket = Socket::create();
		m_client = nullptr;
		m_port = _openRandomPort();
	}
	~HttpReceiver() noexcept
	{
		delete m_client;
		delete m_socket;
	}

	uint getPort() noexcept
	{
		return m_port;
	}

	void accept(EventHandle * canceler) throws(ThrowAbort)
	{
		if (m_port == 0) throw ThrowAbort();
		SocketEvent serverEvent;
		serverEvent->select(m_socket, FNetworkEvent(false, false, false, false, true));
		if (!serverEvent->waitWith(canceler)) throw ThrowAbort();
		serverEvent->getState(m_socket);
		serverEvent->deselect(m_socket);
		m_client = m_socket->accept();
		m_is.resetIStream(m_client);
	}

	void accept(View<EventHandle *> cancelers) throws(ThrowAbort)
	{
		if (m_port == 0) throw ThrowAbort();
		SocketEvent serverEvent;
		serverEvent->select(m_socket, FNetworkEvent(false, false, false, false, true));
		
		EventList<3> events;
		events.push(&serverEvent);
		events.push(cancelers);

		if (events.wait() != 0) throw ThrowAbort();
		serverEvent->getState(m_socket);
		serverEvent->deselect(m_socket);
		m_client = m_socket->accept();
		m_is.resetIStream(m_client);
	}

	void done() noexcept
	{
		Text text = "HTTP/1.1 200 Ok\r\n"
			"Date: Wed, 09 Jan 2019 10:34:53 GMT\r\n"
			"Content-Type: text/plain\r\n"
			"Server: KEN\r\n"
			"Connection: close\r\n"
			"\r\n";
		m_client->writeImpl(text.data(), text.size());
	}

	Text readPosted() noexcept
	{
		m_is.skipwith("Content-Length: ");
		size_t length = m_is.readwith("\r\n").to_uint();
		m_is.skipwith("\r\n\r\n");
		m_data.resize(length);
		char * dest = m_data.data();
		while (length != 0)
		{
			size_t readed = m_is.read(dest, length);
			dest += readed;
			length -= readed;
		}

		//size_t Socket::readFully(ptr binary, size_t len)
		//{
		//	byte* cur = (byte*)binary;
		//	while (len != 0)
		//	{
		//		size_t readed = readImpl(cur, len);
		//		cur += readed;
		//		len -= readed;
		//	}
		//	return cur - (byte*)binary;
		//}
		return m_data;
	}

	void redirectTo(Text url) noexcept
	{
		TSZ text;
		text << "HTTP/1.1 303 See Other\r\n" 
			"Location: " << url << "\r\n"
			"\r\n";
		m_client->writeImpl(text.data(), text.size());
	}
};

namespace
{
	void callGoogleAuth(DeferredPromise<void> * result, EventHandle * canceler) noexcept;
	void callGoogleRefreshToken(DeferredPromise<void> * result, EventHandle * canceler) noexcept
	{
		if (googleRefreshToken == nullptr || googleYouTubeCookie == nullptr)
		{
			return callGoogleAuth(result, canceler);
		}

		HttpRequest req;
		req.setRequestHeader("Content-Type: application/x-www-form-urlencoded");
		req.setPostFields(AText() <<
			"client_id=" GOOGLE_ID "&"
			"client_secret=" GOOGLE_SECRET "&"
			"refresh_token=" << googleRefreshToken << "&"
			"grant_type=refresh_token"
		);

		req.fetchAsText("https://www.googleapis.com/oauth2/v4/token")->then([result, canceler](Text response) {
			int expiresIn = parseToken(response);
			if (expiresIn == -1) return callGoogleAuth(result, canceler);
			if (result) result->_resolve();

			EventPump::getInstance()->postL(expiresIn * 1_s, [canceler](void*){
				callGoogleRefreshToken(nullptr, canceler);
			});
		})->katch([result, canceler](Exception&) {
			return callGoogleAuth(result, canceler);
		});
	}
	void callGoogleAuth(DeferredPromise<void> * result, EventHandle * canceler) noexcept
	{
		threading([canceler]() {

			// prepare key
			AText code;
			AText code_verifier = randomBase64(32);

			{
				AText code_challenge = (encoder::Base64)(TText)(encoder::Sha256)code_verifier;
				reformBase64(code_challenge);

				HttpReceiver recv;
				Process process;
				// TODO: reuqest url directly
				process.exec(TSZ16() <<
					u"res/nwjs-v0.35.4-win-x64/nw.exe "
					u"res/loginpage "
					u"--user-data-dir=" USER_DATA_URL u" "
					u"--target=google "
					u"--scope=" SCOPES u" "
					u"--client_id=" GOOGLE_ID u" "
					u"--code_challenge=" << utf8ToUtf16(code_challenge) << u" "
					u"--port=" << recv.getPort());

				recv.accept({ canceler, process.getEventHandle() });
				Text response = recv.readPosted();
				JsonParser parser(&response);
				parser.fields([&](JsonField & field) {
					field("response", [&](JsonField & field) {
						field("code", &code);
					});
					field("cookie", &googleYouTubeCookie);
				});
				recv.done();
			}

			File::saveFromArray<char>(GOOGLE_COOKIE_FILE, googleYouTubeCookie);


			// builds the  request
			HttpRequest req;
			req.setRequestHeader("Content-Type: application/x-www-form-urlencoded");
			req.setPostFields(AText() <<
				"code=" << code << "&"
				"client_id=" GOOGLE_ID "&"
				"client_secret=" GOOGLE_SECRET "&"
				"grant_type=authorization_code&"
				"redirect_uri=https://rua.kr/webapp/oauth-tunnel-nwjs/&"
				"code_verifier=" << code_verifier);
			AText response = req.fetchAsTextSync("https://www.googleapis.com/oauth2/v4/token");
			return parseToken(response);
		})->then([result, canceler](int expiresIn) {
			if (expiresIn == -1)
			{
				debug();
				return;
			}
			EventPump::getInstance()->postL(expiresIn * 1_s, [canceler](EventPump::Timer * timer) {
				callGoogleRefreshToken(nullptr, canceler);
			});
			result->_resolve();
		})->katch([result](exception_ptr e) {
			result->_rejectException(move(e));
		});
	}

}


Promise<void>* gapi::auth(EventHandle * canceler) noexcept
{
	if (googleAccessToken != nullptr) return Promise<void>::resolve();
	DeferredPromise<void> * result = new DeferredPromise<void>;
	try
	{
		googleRefreshToken = File::openAsArray<char>(GOOGLE_REFRESH_TOKEN_FILE);
		googleYouTubeCookie = File::openAsArray<char>(GOOGLE_COOKIE_FILE);
		callGoogleRefreshToken(result, canceler);
	}
	catch (...)
	{
		callGoogleAuth(result, canceler);
	}
	return result;
}
Promise<void>* gapi::revoke() noexcept
{
	return threading([]() {
		if (googleAccessToken == nullptr) return;
		HttpRequest req;
		googleAccessToken = nullptr;
		googleRefreshToken = nullptr;
		req.setRequestHeader("Content-Type: application/x-www-form-urlencoded");
		req.fetchAsTextSync((TSZ() << "https://accounts.google.com/o/oauth2/revoke?token=" << googleAccessToken));
	});
}

Text gapi::getAccessToken() noexcept
{
	return googleAccessToken;
}
Promise<AText>* gapi::getTokenInfo() noexcept
{
	return fetchAsText(TSZ() << "https://www.googleapis.com/oauth2/v1/tokeninfo?access_token=" << googleAccessToken);
}
Promise<AText16>* gapi::download(const char * url, Text16 ext) noexcept
{
	AText16 filename = getTempFileName(ext);
	Text16 filenameptr = filename;
	return fetchAsFileFromSz(url, filenameptr)->then([filename = move(filename)]() mutable{
		return move(filename);
	});
}
AText gapi::callSync(const char * url, AText data) throws (HttpException)
{
	HttpRequest req;
	if (data != nullptr)
	{
		req.setPostFields(data);
		req.setRequestHeader("Content-Type: application/json; charset=utf-8");
	}
	req.setRequestHeader(TSZ() << "Authorization: Bearer " << googleAccessToken);
	return req.fetchAsTextSync(url);
}
Promise<AText> * gapi::call(const char * url, AText data) noexcept
{
	HttpRequest req;
	if (data != nullptr)
	{
		req.setPostFields(data);
		req.setRequestHeader("Content-Type: application/json; charset=utf-8");
	}
	req.setRequestHeader(TSZ() << "Authorization: Bearer " << googleAccessToken);
	return req.fetchAsText(url);
}
Promise<AText> * gapi::callDelete(const char * url) noexcept
{
	HttpRequest req;
	req.setMethod("DELETE");
	req.setRequestHeader(TSZ() << "Authorization: Bearer " << googleAccessToken);
	return req.fetchAsText(url);
}
Promise<AText16>* gapi::tts(Text text) noexcept
{
	return call("https://texttospeech.googleapis.com/v1beta1/text:synthesize",
		AText() << "{"
		"'input':{"
		"'text':'" << addSlashes(text) << "'"
		"},"
		"'voice':{"
		"'languageCode':'ko-KR',"
		"'ssmlGender':'FEMALE'"
		"},"
		"'audioConfig':{"
		"'audioEncoding':'MP3'"
		"}"
		"}")->then([](Text response) {

		AText audioContent;
		{
			JsonParser parser(&response);
			try
			{
				parser.object([&](Text name) {
					if (name != "audioContent")
					{
						parser.skipValue();
						return;
					}
					audioContent = parser.text();
				});
			}
			catch (...)
			{
			}
		}
		if (audioContent == nullptr)
		{
			throw InvalidSourceException();
		}

		AText result = encoder::Base64::Decoder((Text)audioContent);

		AText16 filename = getTempFileName(u"mp3");
		File::saveFromArray(filename.data(), (Text)result);
		return filename;
	});
}

void gapi::youtube::liveChatBan::remove(Text banId) noexcept
{
	AText send;
	callDelete(TSZ() << "https://www.googleapis.com/youtube/v3/liveChat/bans?id=" << banId);
}
void gapi::youtube::liveChatBan::insert(Text liveChatId, Text channelId, uint duration) noexcept
{
	AText send;
	send << "{"
		"'snippet':{";
	if (duration)
	{
		send << "'banDurationSeconds':" << duration << ",";
	}
	send << "'liveChatId':'" << liveChatId << "',"
			"'type':'";
	if (duration) send << "temporary";
	else send << "permanent";
	send << "',"
			"'bannedUserDetails':{"
				"'channelId':'" << channelId << "'"
			"}"
		"}"
	"}";

	call("https://www.googleapis.com/youtube/v3/liveChat/bans?part=snippet", move(send))->then([](Text respose) {
	});
}
Promise<gapi::youtube::liveChatMessage::List> * gapi::youtube::liveChatMessage::list(Text liveChatId, Text part, Text pageToken) noexcept
{
	AText url;
	url.reserve(512);
	url << "https://www.googleapis.com/youtube/v3/liveChat/messages?"
		"liveChatId=" << (encoder::Uri)liveChatId
		<< "&part=" << (encoder::Uri)part;
	if (pageToken != nullptr && !pageToken.empty()) url << "&pageToken=" << pageToken;
	url.c_str();

	return threading([url = move(url)] {
		AText response = callSync(url.data());
		Text stream = response;
		JsonParser parser(&stream);
		return parser.read<List>();
	});
}
Promise<gapi::youtube::liveBroadcast::List> * gapi::youtube::liveBroadcast::list(Text part, BroadcastType broadcastType, bool mine) noexcept
{
	AText url;
	url.reserve(512);
	url << "https://www.googleapis.com/youtube/v3/liveBroadcasts?part=" << (encoder::Uri)part;
	if (mine) url << "&mine=true";
	switch (broadcastType)
	{
	case BroadcastType::Event: url << "&broadcastType=event"; break;
	case BroadcastType::Persistent: url << "&broadcastType=persistent"; break;
	}
	url.c_str();

	return threading([url = move(url)]{
		AText response = callSync(url.data());
		Text stream = response;
		JsonParser parser(&stream);
		return parser.read<List>();
	});
}
Promise<uint>* gapi::youtube::liveBroadcast::getViewers(Text videoId) noexcept
{
	return fetchAsText(TSZ() << "https://www.youtube.com/live_stats?v=" << videoId)->then([](Text text) {
		if (text.empty()) return (uint)-1;
		return (uint)text.to_uint();
	});
}
Promise<gapi::youtube::video::List>* gapi::youtube::video::list(Text id, Text part) noexcept
{
	AText url;
	url.reserve(512);
	url << "https://www.googleapis.com/youtube/v3/videos?id=" << (encoder::Uri)id;
	url << "&part=" << (encoder::Uri)part;
	url.c_str();

	return threading([url=move(url)](){
		AText response = callSync(url.data());
		Text stream = response;
		JsonParser parser(&stream);
		return parser.read<List>();
	});
}
void gapi::youtube::video::getLiveVideo(Text channelId) noexcept
{
	AText url;
	url.reserve(512);
	url << "https://www.googleapis.com/youtube/v3/search?"
		"part=snippet&"
		"channelId=" << channelId << "&"
		"eventType=live&"
		"type=video";
	url.c_str();

	threading([url = move(url)](){
		AText response = callSync(url.data());
		Text stream = response;
	});
}

AText twitch::callSync(const char * url, AText data) throws(HttpException)
{
	HttpRequest req;
	if (data != nullptr)
	{
		req.setPostFields(data);
		req.setRequestHeader("Content-Type: application/x-www-form-urlencoded");
	}
	req.setRequestHeader("Accept: application/vnd.twitchtv.v5+json");
	req.setRequestHeader("Client-ID: " TWITCH_ID);
	req.setRequestHeader(TSZ() << "Authorization: OAuth " << twitchAccessToken);
	return req.fetchAsTextSync(url);
}
Promise<void>* twitch::auth(EventHandle * canceler, AText scope) noexcept
{
	if (twitchAccessToken != nullptr) return Promise<void>::resolve();
	return threading([canceler, scope = move(scope)] {
		try
		{
			twitchAccessToken = File::openAsArray<char>(TWITCH_ACCESS_TOKEN_FILE);
			return;
		}
		catch (...)
		{
		}

		HttpReceiver recv;
		Process process;
		process.exec(TSZ16() <<
			u"res/nwjs-v0.35.4-win-x64/nw.exe "
			u"res/loginpage "
			u"--user-data-dir=" USER_DATA_URL u" "
			u"--target=twitch "
			u"--client_id=" TWITCH_ID u" "
			u"--scope=\"" << utf8ToUtf16(scope) << u"\" "
			u"--port=" << recv.getPort()
		);

		recv.accept({ canceler, process.getEventHandle() });
		Text response = recv.readPosted();
		JsonParser parser(&response);

		parser.fields([](JsonField & field){
			field("access_token", &twitchAccessToken);
			field("id_token", &twitchIdToken);
		});
		recv.done();

		File::saveFromArray<char>(TWITCH_ACCESS_TOKEN_FILE, (Text)twitchAccessToken);

		/*

		Process::executeOpen(TSZ16() << "https://id.twitch.tv/oauth2/authorize?"
			u"client_id=" TWITCH_ID
			u"&redirect_uri=https://rua.kr/webapp/oauth-tunnel/"
			u"&state=" << oauth.getPort() <<
			u"&response_type=token+id_token"
			u"&scope=" << utf8ToUtf16(scope)
		);

		io::SelfBufferedIStream<io::SocketStream<char>> stream = oauth.accept(canceler)->stream<char>();
		Text line = stream.readwith("\r\n");
		line.readwith(' ');
		line.skip(2);
		twitchAccessToken = line.readwith('&');
		File::saveFromArray<char>(TWITCH_ACCESS_TOKEN_FILE, (Text)twitchAccessToken);
		twitchIdToken = line.readwith(' ');
		stream->write(
			"HTTP/1.1 303 See Other\r\n"
			"Location: https://rua.kr/webapp/oauth-ok/\r\n"
			"\r\n");
		*/
	});
}
Promise<void>* twitch::revoke() noexcept
{
	return threading([]() {
		if (twitchAccessToken == nullptr) return;
		HttpRequest req;
		req.fetchAsTextSync(TSZ() << "https://id.twitch.tv/oauth2/revoke?client_id=" << TWITCH_ID << "&token=" << twitchAccessToken);
		twitchAccessToken = nullptr;
	});
}
Text twitch::getAccessToken() noexcept
{
	return twitchAccessToken;
}
Promise<void>* twitch::loadBadges() noexcept
{
	return s_badges.load();
}
Text twitch::getBadgeUrl(Text name, uint version) noexcept
{
	return s_badges.getUrl(name, version);
}
Promise<twitch::Channel>* twitch::getChannel() noexcept
{
	return threading([](){
		AText response = callSync("https://api.twitch.tv/kraken/channel");
		Text stream = response;
		JsonParser parser(&stream);
		return parser.read<Channel>();
	});
}
Promise<twitch::Channel>* twitch::getChannelById(AText channelId) noexcept
{
	return threading([channelId = move(channelId)]() {
		AText response = callSync(TSZ() << "https://api.twitch.tv/kraken/channels/" << channelId);
		Text stream = response;
		JsonParser parser(&stream);
		return parser.read<Channel>();
	});
}
Promise<twitch::ChatRooms>* twitch::getChatRooms(AText channelId) noexcept
{
	return threading([channelId = move(channelId)]() {
		TSZ url;
		url.reserve(512);
		url << "https://api.twitch.tv/kraken/chat/" << channelId << "/rooms";
		AText response = callSync(url);
		Text stream = response;

		JsonParser parser(&stream);
		return parser.read<ChatRooms>();
	});
}
Promise<twitch::Stream>* twitch::getStreamByUser(AText channelId) noexcept
{
	return threading([channelId = move(channelId)]() {
		TSZ url;
		url.reserve(512);
		url << "https://api.twitch.tv/kraken/streams/" << channelId;
		AText response = callSync(url);
		Text stream = response;

		JsonParser parser(&stream);
		return parser.read<Stream>();
	});
}
