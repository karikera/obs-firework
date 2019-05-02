#pragma once
#include <KR3/main.h>
#include <KRUtil/parser/jsonparser.h>
namespace kr
{
	namespace twitch
	{
		struct Channel
		{
			bool mature = false;
			kr::AText status;
			kr::AText broadcaster_language;
			kr::AText display_name;
			kr::AText game;
			kr::AText language;
			kr::AText _id;
			kr::AText /** karikera3 */ name;
			kr::AText created_at;
			kr::AText updated_at;
			bool partner = false;
			kr::AText logo;
			kr::AText video_banner;
			kr::AText profile_banner;
			kr::AText profile_banner_background_color;
			kr::AText url;
			uint views = 0;
			uint followers = 0;
			struct _links_t
			{
				kr::AText self;
				kr::AText follows;
				kr::AText commercial;
				kr::AText stream_key;
				kr::AText chat;
				kr::AText features;
				kr::AText subscriptions;
				kr::AText editors;
				kr::AText teams;
				kr::AText videos;
				void parseJson(JsonParser & parser)
				{
					parser.fields([this](JsonField& field)
					{
						field("self", &self);
						field("follows", &follows);
						field("commercial", &commercial);
						field("stream_key", &stream_key);
						field("chat", &chat);
						field("features", &features);
						field("subscriptions", &subscriptions);
						field("editors", &editors);
						field("teams", &teams);
						field("videos", &videos);
					}
					);
				}
			};
			_links_t _links;
			kr::AText delay;
			kr::AText banner;
			kr::AText background;
			kr::AText broadcaster_type;
			kr::AText stream_key;
			kr::AText email;
			void parseJson(JsonParser & parser)
			{
				parser.fields([this](JsonField& field)
				{
					field("mature", &mature);
					field("status", &status);
					field("broadcaster_language", &broadcaster_language);
					field("display_name", &display_name);
					field("game", &game);
					field("language", &language);
					field("_id", &_id);
					field("name", &name);
					field("created_at", &created_at);
					field("updated_at", &updated_at);
					field("partner", &partner);
					field("logo", &logo);
					field("video_banner", &video_banner);
					field("profile_banner", &profile_banner);
					field("profile_banner_background_color", &profile_banner_background_color);
					field("url", &url);
					field("views", &views);
					field("followers", &followers);
					field("_links", &_links);
					field("delay", &delay);
					field("banner", &banner);
					field("background", &background);
					field("broadcaster_type", &broadcaster_type);
					field("stream_key", &stream_key);
					field("email", &email);
				}
				);
			}
		};
		struct ChatRooms
		{
			uint _total = 0;
			struct rooms_t
			{
				kr::AText _id;
				kr::AText owner_id;
				kr::AText name;
				kr::AText topic;
				bool is_previewable = false;
				kr::AText minimum_allowed_role;
				void parseJson(JsonParser & parser)
				{
					parser.fields([this](JsonField& field)
					{
						field("_id", &_id);
						field("owner_id", &owner_id);
						field("name", &name);
						field("topic", &topic);
						field("is_previewable", &is_previewable);
						field("minimum_allowed_role", &minimum_allowed_role);
					}
					);
				}
			};
			Array<rooms_t> rooms;
			void parseJson(JsonParser & parser)
			{
				parser.fields([this](JsonField& field)
				{
					field("_total", &_total);
					field("rooms", &rooms);
				}
				);
			}
		};
		struct Stream
		{
			struct stream_t
			{
				uint _id = 0;
				kr::AText game;
				uint viewers = -1;
				uint video_height = 0;
				uint average_fps = 0;
				uint delay = 0;
				kr::AText created_at;
				bool is_playlist = false;
				struct preview_t
				{
					kr::AText small_;
					kr::AText medium;
					kr::AText large;
					kr::AText template_;
					void parseJson(JsonParser & parser)
					{
						parser.fields([this](JsonField& field)
						{
							field("small", &small_);
							field("medium", &medium);
							field("large", &large);
							field("template", &template_);
						}
						);
					}
				};
				preview_t preview;
				struct channel_t
				{
					bool mature = false;
					kr::AText status;
					kr::AText broadcaster_language;
					kr::AText display_name;
					kr::AText game;
					kr::AText language;
					uint _id = 0;
					kr::AText name;
					kr::AText created_at;
					kr::AText updated_at;
					bool partner = false;
					kr::AText logo;
					kr::AText video_banner;
					kr::AText profile_banner;
					kr::AText profile_banner_background_color;
					kr::AText url;
					uint views = 0;
					uint followers = 0;
					void parseJson(JsonParser & parser)
					{
						parser.fields([this](JsonField& field)
						{
							field("mature", &mature);
							field("status", &status);
							field("broadcaster_language", &broadcaster_language);
							field("display_name", &display_name);
							field("game", &game);
							field("language", &language);
							field("_id", &_id);
							field("name", &name);
							field("created_at", &created_at);
							field("updated_at", &updated_at);
							field("partner", &partner);
							field("logo", &logo);
							field("video_banner", &video_banner);
							field("profile_banner", &profile_banner);
							field("profile_banner_background_color", &profile_banner_background_color);
							field("url", &url);
							field("views", &views);
							field("followers", &followers);
						}
						);
					}
				};
				channel_t channel;
				void parseJson(JsonParser & parser)
				{
					parser.fields([this](JsonField& field)
					{
						field("_id", &_id);
						field("game", &game);
						field("viewers", &viewers);
						field("video_height", &video_height);
						field("average_fps", &average_fps);
						field("delay", &delay);
						field("created_at", &created_at);
						field("is_playlist", &is_playlist);
						field("preview", &preview);
						field("channel", &channel);
					}
					);
				}
			};
			stream_t stream;
			void parseJson(JsonParser & parser)
			{
				parser.fields([this](JsonField& field)
				{
					field("stream", &stream);
				}
				);
			}
		};
	}
}
