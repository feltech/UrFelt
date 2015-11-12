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

#include "UrSurface3D.hpp"


extern int tolua_UrFelt_open (lua_State* tolua_S);


namespace felt
{
	class UrFelt : public Urho3D::Application
	{
	public:
		typedef Surface<3>	Surface_t;
	protected:
		UrSurface3D		m_surface;
		Urho3D::RigidBody* 		m_surface_body;
		float			m_time_since_update;

		enum UpdaterState {
			STOPPED, RUNNING, STOP, PAUSE, PAUSED, ZAP, REPOLY
		};

		std::thread 				m_thread_updater;
		std::atomic<UpdaterState>	m_state_updater;
		std::mutex					m_mutex_updater;
		std::condition_variable		m_cond_updater;

		struct Zapper {
			float pos[3];
			float dir[3];
			float amount;
		};

		struct WorkerMessage
		{
			enum Type
			{
				STOP_ZAP,
				ZAP
			};
			const Type type;
			WorkerMessage(const Type& type_) : type(type_) {}
		};

		struct ZapWorkerMessage : public WorkerMessage
		{
			const Zapper zap;
			ZapWorkerMessage(const Zapper& zap_) : WorkerMessage(ZAP), zap(zap_) {}
		};

		using WorkerMessagePtr = std::shared_ptr<WorkerMessage>;

		std::queue<WorkerMessagePtr>	m_worker_queue;
		std::mutex						m_worker_queue_mutex;

		std::atomic<Zapper> m_zap;

	public:
		~UrFelt();
		UrFelt(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

		void zap(const Urho3D::Ray& ray, const float& amount);

		void repoly();

	protected:
		void Setup();
		void Start();
		void handle_update(
			Urho3D::StringHash eventType, Urho3D::VariantMap& eventData
		);
		void updater();
		void start_updater();
	};

}
#endif /* INCLUDE_URFELT_HPP_ */
