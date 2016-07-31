/*
 * UrFelt.hpp
 *
 *  Created on: 5 Jun 2015
 *      Author: dave
 */

#ifndef INCLUDE_URFELT_HPP_
#define INCLUDE_URFELT_HPP_

#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>

#include <Urho3D/Urho3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/Physics/RigidBody.h>

#include <Felt/Surface.hpp>
#include <LuaCppMsg.hpp>

#include "UrSurface3D.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);


namespace felt
{
	namespace Messages
	{
		enum MsgType
		{
			STATE_RUNNING = 1, ACTIVATE_SURFACE, START_ZAP, STOP_ZAP, PERCENT_TOP, PERCENT_BOTTOM,
			MAIN_INIT_DONE, WORKER_INIT_DONE
		};
	}

	class UrFelt : public Urho3D::Application
	{
	public:
		~UrFelt();
		UrFelt(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

		void repoly();

	protected:
		void Setup();
		void Start();
		void handle_update(
			Urho3D::StringHash eventType, Urho3D::VariantMap& eventData
		);
		void updater();
		void start_updater();

	protected:
		using UrQueue = LuaCppMsg::Queue<
			Urho3D::Ray, LuaCppMsg::CopyPtr<Urho3D::Ray>*, float
		>;

		UrSurface3D				m_surface;
		Urho3D::RigidBody* 		m_surface_body;
		float					m_time_since_update;
		enum State {
			INIT, INIT_DONE, RUNNING,
			STOPPED, STOP, PAUSE, PAUSED, ZAP, REPOLY
		};

		std::thread 				m_thread_updater;
		State						m_state_main;
		std::atomic<State>			m_state_updater;
		std::atomic<bool>			m_quit;
		std::mutex					m_mutex_updater;
		std::condition_variable		m_cond_updater;

		UrQueue	m_queue_script;
		UrQueue	m_queue_worker;
		UrQueue	m_queue_main;
	};

}
#endif /* INCLUDE_URFELT_HPP_ */
