#include "stdafx.h"
#include "config.h"

#include <KRUtil/fs/file.h>
#include <KR3/io/selfbufferedstream.h>

Config g_config;

void TestMessage::parseJson(JsonParser & parser)
{
	parser.fields([this](JsonField & field) {
		field("name", &name);
		field("message", &message);
		field("icon", &icon);
		field("tier", &tier);
		field("amount", &amount);
		field("count", &count);
	});
}

void Config::load() noexcept
{
	testMode = false;
	chatDuration = 30.f;
	normalChatFontSize = 16.f;
	superChatFontSize = 28.f;
	normalChatWidth = 212.f;

	try
	{
		io::FIStream<char> fis = File::open(u"res/config.json");
		fis.hasBom();
		JsonParser parser(&fis);
		parser.fields([this](JsonField & field) {
			field("donateSound", &donateSound);
			field("twitchImage", &twitchImage);
			field("bgmPath", &bgmPath);
			field("testMessages", &testMessages);
			field("testMode", &testMode);
			field("enableYoutube", &enableYoutube);
			field("enableTwitch", &enableTwitch);
			field("characterSpace", &characterSpace);
			field("normalChatFontSize", &normalChatFontSize);
			field("normalChatWidth", &normalChatWidth);
			field("superChatFontSize", &superChatFontSize);
			field("chatDuration", &chatDuration);
		});
		donateSound.c_str();
	}
	catch (...)
	{
		debug(); // Invalid config
	}
	if (!testMode)
	{
		testMessages = nullptr;
	}
}
