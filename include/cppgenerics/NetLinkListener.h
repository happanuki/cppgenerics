#pragma once

#include <linux/rtnetlink.h>
#include <net/if.h>

#include <exception>
#include <thread>
#include <mutex>
#include <list>

#include "cppgenerics/interfaces/Observer_I.h"

namespace CppGenerics {

namespace System {

enum NetLinkInterfaceEvents
{
	NONE,
	DEL_ADDR = RTM_DELADDR, //network address was deleted
	DEL_LINK = RTM_DELLINK, //iface deleted
	ADD_ADDR = RTM_NEWADDR, //new address on iface
	ADD_LINK = RTM_NEWLINK, //new iface

};

class NetLinkListener
{
public:
	struct InterfaceEventCB_t
	{
		NetLinkInterfaceEvents event;
		std::string ifaceName;
		std::function< void(void*) > callback; //cbArg
		void* cbArg;

		bool operator==(const InterfaceEventCB_t o) {
			return 	( event == o.event )  &&
					( ifaceName == ifaceName);
		}

	};

private:
	std::mutex m_ifaceEvsMtx;
	std::list<InterfaceEventCB_t> m_ifaceEvs;

	sockaddr_nl m_localAddr = { 0 };
	std::thread m_ifaceEventsProcessor;
	bool m_ifaceEventProcessor_started = false;
	void m_ifaceEventProcessor_f();

	NetLinkListener();

	int m_netlinkSocket = -1;
	void openNetlinkSocket() throw (std::exception&);

	void startProcessor() throw (std::exception&);

	void parseRtattr(rtattr *tb[], int max, rtattr *rta, int len) noexcept;
	void processEvent(NetLinkInterfaceEvents ev,const char* ifName) throw (std::exception&);

public:
	NetLinkListener(const NetLinkListener&) = delete;
	NetLinkListener& operator=(const NetLinkListener&) = delete;

	static NetLinkListener& getInstance() throw (std::exception&);

	void addInterfaceEvent(const InterfaceEventCB_t& in) throw (std::exception&);
	void removeInterfaceEvent(const InterfaceEventCB_t& ev) throw (std::exception&);

	~NetLinkListener();
};

} //namespace

} // namespace CppGenerics
