#pragma once

#include <KRUtil/parser/jsonparser.h>

using namespace kr;

struct TestMessage
{
	AText name;
	AText message;
	AText icon;
	int tier = 0;
	AText amount;
	uint count = 1;

	void parseJson(JsonParser & parser);
};


class Config
{
public:
	AText16 donateSound;
	AText16 twitchImage;
	AText16 bgmPath;
	Array<TestMessage> testMessages;
	bool testMode;
	bool enableYoutube;
	bool enableTwitch;
	float characterSpace;
	float chatDuration;
	float normalChatFontSize;
	float normalChatWidth;
	float superChatFontSize;

	void load() noexcept;
};

extern Config g_config;
