#pragma once

#include <KR3/main.h>

struct ID3D11Texture2D;

class FireworkSystem
{
public:
	~FireworkSystem() noexcept;

	void createWindowFrom(ID3D11Texture2D * texture, int textureWidth, int textureHeight) noexcept;
	void destroy() noexcept;
	
	void update() noexcept;
	void render(double time) noexcept;
	void mouseDown() noexcept;
	void mouseUp() noexcept;
	void mouseMove(kr::ivec2 pos) noexcept;
	void mouseOut() noexcept;

	static FireworkSystem* create(double startTime) noexcept;

private:
	FireworkSystem() noexcept;


#ifdef FIREWORK_SYSTEM_IMPL

	void _createDevice(int textureWidth, int textureHeight) noexcept;
	void _destroyDevice() noexcept;
	void _init() noexcept;
	void _connect() noexcept;
	void _update() noexcept;

	kr::PromiseManager * m_promMgr;
	kr::EventPump * m_pump;
	kr::StackAllocator * m_stackAllocator;
	kr::EventHandle * m_canceler;
	MusicPlayer m_player;
	Vote m_vote;
	ID3D11Texture2D * m_targetTexture;
	ChattingPusher * m_chatPusher;
	bool m_started;
	bool m_resourceReady;
	bool m_connected;
#endif

};
