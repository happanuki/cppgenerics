#include <unistd.h>

#include <memory>

#include "cppgenerics/NetLinkListener.h"
#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"

namespace CppGenerics {

namespace System {


std::unique_ptr<NetLinkListener> holder(nullptr);

NetLinkListener& NetLinkListener::getInstance() throw (std::exception&)
{
	static std::mutex s_mtx;

	std::lock_guard<std::mutex> g(s_mtx);
	if ( holder == nullptr ) {

		(void) atexit( [](){ holder.reset(); } );

		holder.reset( new NetLinkListener);
		if ( holder == nullptr) {
			THROW_SYS_EXCEPTION("malloc failed");
		}

		holder->openNetlinkSocket();
		holder->startProcessor();
	}

	return *holder;
}

void NetLinkListener::openNetlinkSocket() throw (std::exception&)
{

}

NetLinkListener::NetLinkListener()
{

}


NetLinkListener::~NetLinkListener()
{
	::close(m_netlinkSocket); //makes m_ifaceEventProcessor_f stop

	if ( m_ifaceEventsProcessor.joinable()) {
		m_ifaceEventsProcessor.join();
	}
}

void NetLinkListener::m_ifaceEventProcessor_f()
{
	m_netlinkSocket = System::socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	m_localAddr.nl_family = AF_NETLINK; // указываем семейство протокола
	m_localAddr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE; // указываем необходимые группы
	m_localAddr.nl_pid = pthread_self() << 16 | getpid(); //в качестве идентификатора указываем идентификатор данного приложения
							//(pthread_self() << 16 | getpid();)

	System::bind(m_netlinkSocket, (struct sockaddr*)&m_localAddr, sizeof(m_localAddr));

	const int l_bufSize = 8192;

	iovec l_iovec = { nullptr, l_bufSize };

	std::unique_ptr< uint8_t, std::function< void(uint8_t*) >>
			buf(new uint8_t[l_bufSize],std::default_delete<uint8_t[]>());

	if (buf == nullptr) {
		THROW_SYS_EXCEPTION("malloc failed");
	}

	l_iovec.iov_base = buf.get();

	// структура сообщения netlink – инициализируем все поля
	msghdr msg = {
		.msg_name = &m_localAddr, 			// задаем имя – структуру локального адреса
		.msg_namelen = sizeof(m_localAddr), 	// указываем размер
		.msg_iov = &l_iovec, 			// указываем вектор данных сообщения
		.msg_iovlen = 1 				// задаем длину вектора
	};

	int ret;
	while ( (ret = System::recvmsg( m_netlinkSocket, &msg, 0))) {

		nlmsghdr* h = reinterpret_cast<nlmsghdr*>(buf.get()); // указатель на заголовок сообщения

		DEBUGSTDOUTT("MSG!");

		if ((h->nlmsg_type == RTM_NEWROUTE) || (h->nlmsg_type == RTM_DELROUTE)) {
			LOGSTDOUTT("[ NetLink ] Routing table changes");
		}
		else {
			rtattr* tb[IFLA_MAX + 1] = { 0 }; // массив атрибутов соединения, IFLA_MAX определен в rtnetlink.h

			ifinfomsg *ifi = (ifinfomsg*) NLMSG_DATA(h); // получаем информацию о сетевом соединении в котором произошли изменения

			parseRtattr(tb, IFLA_MAX, IFLA_RTA(ifi), h->nlmsg_len); // получаем атрибуты сетевого соединения

			const char* ifName;
			bool isUp, isRun;

			if (tb[IFLA_IFNAME]) { // проверяем валидность атрибута, хранящего имя соединения
				ifName = (const char*)RTA_DATA(tb[IFLA_IFNAME]); //получаем имя соединения
			}

			std::string ifName_s("none");
			if (ifName != nullptr) {
				ifName_s = std::string(ifName);
			}

			if (ifi->ifi_flags & IFF_UP) { // получаем состояние флага UP для соединения
				isUp = true;
			} else {
				isUp = false;
			}

			if (ifi->ifi_flags & IFF_RUNNING) { // получаем состояние флага RUNNING для соединения
				isRun = true;
			} else {
				isRun = false;
			}

			DEBUGSTDOUTT("ifName_s : " + ifName_s);

			rtattr* tba[IFLA_MAX + 1] = { 0 }; // массив атрибутов адреса
			struct ifaddrmsg *ifa = (ifaddrmsg*) NLMSG_DATA(h); // получаем данные из соединения

			const int textIpAddrLen = 50;
			std::unique_ptr< uint8_t, std::function<void(uint8_t*)>>
					addrBuff( new uint8_t[textIpAddrLen],std::default_delete<uint8_t[]>());

			parseRtattr(tba, IFA_MAX, IFA_RTA(ifa), h->nlmsg_len); // получаем атрибуты сетевого соединения

			if (tba[IFA_LOCAL]) { // проверяем валидность указателя локального адреса
				inet_ntop(AF_INET, RTA_DATA(tba[IFA_LOCAL]), (char*)addrBuff.get(), textIpAddrLen); // получаем IP адрес
			}

			switch (h->nlmsg_type) { //что конкретно произошло
			case RTM_DELADDR:
				LOGSTDOUTT(" [ NetLink ] Address removed : " +ifName_s);
				processEvent(NetLinkInterfaceEvents::DEL_ADDR,ifName);
				break;

			case RTM_DELLINK:
				LOGSTDOUTT(" [ NetLink ] Link down : " + ifName_s);
				processEvent(NetLinkInterfaceEvents::DEL_LINK,ifName);
				break;

			case RTM_NEWLINK:
				LOGSTDOUTT(" [ NetLink ] Link added : " + ifName_s);
				processEvent(NetLinkInterfaceEvents::ADD_LINK,ifName);
				break;

			case RTM_NEWADDR:
				LOGSTDOUTT(" [ NetLink ] New address ("+std::string( (const char*)addrBuff.get()) + ") : " + ifName_s);
				processEvent(NetLinkInterfaceEvents::ADD_ADDR,ifName);
				break;

			default:
				DEBUGSTDOUTT(" MSG: " + std::to_string(h->nlmsg_type));
			}
		}
	}
}

void NetLinkListener::processEvent(NetLinkInterfaceEvents ev,const char* ifName) throw (std::exception&)
{
	std::lock_guard<std::mutex> g(m_ifaceEvsMtx);

	std::string ifaceStr(ifName);

	for (auto& it: m_ifaceEvs) {
		if (( it.event == ev) && ( it.ifaceName == ifaceStr )) {
			it.callback(it.cbArg);
		}
	}
}

void NetLinkListener::startProcessor() throw (std::exception&)
{
	m_ifaceEventProcessor_started = true;
	m_ifaceEventsProcessor = std::move( std::thread( &NetLinkListener::m_ifaceEventProcessor_f, this));
}

/*
 * 	небольшая вспомогательная функция, которая с помощью макрасов netlink разбирает сообщение и
 * 	помещает блоки данных в массив атрибутов rtattr
 */
void NetLinkListener::parseRtattr(rtattr *tb[], int max, rtattr *rta, int len) noexcept
{
//	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) { // пока сообщение не закончилось
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta; //читаем атрибут
		}
		rta = RTA_NEXT(rta,len); // получаем следующий атрибут
	}
}

void NetLinkListener::addInterfaceEvent(const InterfaceEventCB_t& in) throw (std::exception&)
{
	std::lock_guard<std::mutex> g(m_ifaceEvsMtx);
	m_ifaceEvs.push_back(in);
}

void NetLinkListener::removeInterfaceEvent(const InterfaceEventCB_t& in) throw (std::exception&)
{
	std::lock_guard<std::mutex> g(m_ifaceEvsMtx);
	m_ifaceEvs.remove(in);
}

} //namespace

} // namespace CppGenerics
