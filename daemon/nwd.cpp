#include "nwd.hpp"



namespace nfd {
	namespace gateway{
//	NFD_LOG_INIT("Nwd");
	const ndn::time::milliseconds Nwd::DEFAULT_EXPIRATION_PERIOD = ndn::time::milliseconds::max();
	const uint64_t Nwd::DEFAULT_COST = 0;
    Nwd::RouteTable_Type Nwd::route_table={};
    Nwd::Neighbor_Type Nwd::neighbors_list={};
    Nwd::Reverse_Neighbor_Type Nwd::reverse_neighbors_list={};
    Coordinate Nwd::self;
    Range Nwd::self_range;
    serial_manager  Nwd::m_serialManager;
    time_t Nwd::globe_timestamp(0);
    ndn::Face Nwd::m_face;
    ndn::KeyChain Nwd::m_keyChain;
    std::set<std::string> Nwd::topo_data;
    std::map<std::string,std::set<std::string>> Nwd::interest_list;
    bool Nwd::handle_interest_busy(false);
    boost::asio::steady_timer Nwd::m_t(m_face.getIoService());
    std::set<WsnData> Nwd::data_set;
    int local_timestamp(0);
    std::map<std::string,std::string> Nwd::wsn_location_map;
    std::map<std::string,std::pair<unsigned int,std::string>> Nwd::wifi_location_map;
    int Nwd::local_timestamp(0);
    std::queue<std::string> Nwd::receive_in_queue;
    double Nwd::leftdown_longitude(116.362260),Nwd::leftdown_latitude(39.964100),Nwd::rightup_longitude(116.363760),Nwd::rightup_latitude(39.967130);
    double Nwd::longitude((leftdown_longitude+rightup_longitude)/2),Nwd::latitude((leftdown_latitude+rightup_latitude)/2) ;//网关经纬度
    std::queue<std::string> Nwd::origin_wsn_interestname;
    std::shared_ptr<Forwarder> Nwd::m_forwarder;
    std::queue<std::shared_ptr<boost::asio::steady_timer>> Nwd::timer_queue;
    int  Nwd::time_sync_count=0;

	Nwd::Nwd(nfd::Nfd& nfd) : m_flags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT), m_cost(DEFAULT_COST) ,
		m_origin(ndn::nfd::ROUTE_ORIGIN_STATIC),m_expires(DEFAULT_EXPIRATION_PERIOD),
		m_facePersistency(ndn::nfd::FacePersistency::FACE_PERSISTENCY_PERSISTENT),
		m_controller(m_face, m_keyChain),
		m_tsync(m_face.getIoService()),wifi_num(11),wsn_nodes(0)

	{
        self.set_latitude(latitude);
        self.set_longitude(longitude);
        self_range.set_leftdown(Coordinate(leftdown_longitude,leftdown_latitude));
        self_range.set_rightup(Coordinate(rightup_longitude,rightup_latitude));
        m_forwarder=nfd.get_Forwarder();
	}
	
	void
	Nwd::initialize()
	{
        gateway_area=to_string(leftdown_longitude)+"/"+to_string(leftdown_latitude)+"/"+to_string(rightup_longitude)+"/"+to_string(rightup_latitude);
//		m_forwarder->getStrategyChoice().install(std::make_shared<fw::LocationRouteStrategy>(*m_forwarder,fw::LocationRouteStrategy::STRATEGY_NAME));

		m_face.setInterestFilter("/NDN-IOT-REGISTER/wifi/",
									 bind(&Nwd::Wifi_Register_onInterest, this, _1, _2),
									 ndn::RegisterPrefixSuccessCallback(),
									 bind(&Nwd::onRegisterFailed, this, _1, _2));
		
//		m_face.setInterestFilter("/wifi/location",
//									 bind(&Nwd::Wifi_Location_onInterest, this, _1, _2),
//									 ndn::RegisterPrefixSuccessCallback(),
//									 bind(&Nwd::onRegisterFailed, this, _1, _2));
//
//        m_face.setInterestFilter("/wifi/topo",
//                                 bind(&Nwd::Wifi_Topo_onInterest, this, _1, _2),
//                                 ndn::RegisterPrefixSuccessCallback(),
//                                 bind(&Nwd::onRegisterFailed, this, _1, _2));

//        m_face.setInterestFilter("/NDN-IOT/",
//                                 bind(&Nwd::nfd_location_onInterest,  _1, _2),
//                                 ndn::RegisterPrefixSuccessCallback(),
//                                 bind(&Nwd::onRegisterFailed, this, _1, _2));



		getEthernetFace(); //获取本地网址
		for(auto itr:ethface_map)
		{

			if(strncmp(itr.first.data(),"wlan",4) ==0 )  //如果有wlan接口启动热点
			{
				//启动wifi热点
				threadGroup.create_thread(boost::bind(&Nwd::WlanHotSpotStart,this));

			}
		}


        threadGroup.create_thread(boost::bind(&Nwd::wsnPeriodFind,this)); //发现wsn网络

//        NdpInitialize(); //NDP初始化，发现邻居 （动态发现邻居)
         //静态发现邻居
        neighbors_list.clear();  //清空邻居表
        getNeighborsRange();  //邻居表初始化、更新
        printNeighborsTable();  //打印邻居表
        RoutingtableUpdate();  //更新路由表


        strategyChoiceSet("/NDN-IOT","ndn:/localhost/nfd/strategy/location-route");  //设置路由策略
//        strategyChoiceSet("/NDN-WIFI","ndn:/localhost/nfd/strategy/broadcast");

    	m_face.processEvents();
		
	}

    void
    Nwd::wsnPeriodFind()   //wsn网络接口的周期性发现
    {
        while(true)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(10));
            //读取ＵＳＢ０，后续修改读取多个网络接口
            std::cout<<"start wsn test"<<std::endl;
            int fdusb = open(USB_PATH_PORT, O_RDWR);
            std::cout<<"fdusb is "<<fdusb<<std::endl;
            if(fdusb != -1) {
                std::cout<<"start wsn success"<<std::endl;
//                m_face.setInterestFilter("/wsn/topo",
//                                         bind(&Nwd::Topo_onInterest, this, _1, _2),
//                                         ndn::RegisterPrefixSuccessCallback(),
//                                         bind(&Nwd::onRegisterFailed, this, _1, _2));


                m_face.setInterestFilter("/wsn",
                                         bind(&Nwd::onInterest, this, _1, _2),
                                         ndn::RegisterPrefixSuccessCallback(),
                                         bind(&Nwd::onRegisterFailed, this, _1, _2));

//                m_face.setInterestFilter("/wsn/location",
//                                         bind(&Nwd::Location_onInterest, this, _1, _2),
//                                         ndn::RegisterPrefixSuccessCallback(),
//                                         bind(&Nwd::onRegisterFailed, this, _1, _2));


                threadGroup.create_thread(boost::bind(&Nwd::listen_wsn_data, this, &m_serialManager))->detach();

                threadGroup.create_thread(boost::bind(&Nwd::time_sync_init, this))->detach();
//
                threadGroup.create_thread(boost::bind(&Nwd::manage_wsn_topo, this, &m_serialManager))->detach();
                break;
            }
        }


    }

    //ndp初始化
    void
    Nwd::NdpInitialize()
    {
//
//		getEthernetFace();
        //广播发送
//		face_name="/ndp/discover";
//		std::cout<<"face name:"<<face_name<<std::endl;
//		ribRegisterPrefix();
//        strategyChoiceSet(face_name,"ndn:/localhost/nfd/strategy/broadcast");


        threadGroup.create_thread(boost::bind(&Nwd::ServerListenBroadcast,this));
        threadGroup.create_thread(boost::bind(&Nwd::ClientBroadcast,this));


    }

    void
    Nwd::NeighborsInitialize()   //邻居表初始化 　
    {
        boost::this_thread::sleep(boost::posix_time::seconds(2));
        getNeighborsRange();  //邻居表初始化
        printNeighborsTable();
    }

//    void
//    Nwd::getNeighborsCoordinate()  //初始化、更新邻居表
//    {
//        auto fib_entry_itr=m_forwarder->getFib().begin();
//
//
//        for(;fib_entry_itr!=m_forwarder->getFib().end();fib_entry_itr++){
//            std::ostringstream os;
//            os<<(*fib_entry_itr).getPrefix()<<std::endl;
//            std::string fib_entry_name=os.str();
//            /*查找最近路由接口并转发，目前路由前缀为/nfd,后接地理位置*/
//            if(fib_entry_name.find("/nfd")!=std::string::npos && fib_entry_name.find_first_of("0123456789") != std::string::npos) {
//                std::cout<<"fib name is: "<<fib_entry_name;
//                std::string::size_type pos1=fib_entry_name.find('/',1);
//                std::string::size_type pos2=fib_entry_name.find('/',pos1+1);
//                //      std::string::size_type pos3=fib_entry_name.find('/',pos2+1);
//
//                double position_x = std::stod(fib_entry_name.substr(pos1+1,pos2-pos1-1));
//                double position_y = std::stod(fib_entry_name.substr(pos2+1));
////            std::cout<<"position is ("<<position_x<<","<<position_y<<")"<<std::endl;
//                fib::NextHopList nexthops;
//                nexthops=fib_entry_itr->getNextHops();
//                fib::NextHopList::const_iterator it = nexthops.begin();
//                for (;it != nexthops.end();++it) {
//                    shared_ptr <Face> outFace = it->getFace();
//                    gateway::Nwd::neighbors_list.insert(make_pair(gateway::Coordinate(position_x, position_y), outFace));
//                       gateway::Nwd::reverse_neighbors_list.insert(
//                            make_pair(outFace, gateway::Coordinate(position_x, position_y)));
//
//                }
//            }
//
//        }
//    }


    void
    Nwd::getNeighborsRange()  //初始化、更新邻居表
    {
        auto fib_entry_itr=m_forwarder->getFib().begin();


        for(;fib_entry_itr!=m_forwarder->getFib().end();fib_entry_itr++){
            std::ostringstream os;
            os<<(*fib_entry_itr).getPrefix()<<std::endl;
            std::string fib_entry_name=os.str();
            /*查找最近路由接口并转发，目前路由前缀为/nfd,后接地理位置*/
            if(fib_entry_name.find("/NDN-IOT")!=std::string::npos && fib_entry_name.find_first_of("0123456789") != std::string::npos) {
                std::cout<<"fib name is: "<<fib_entry_name;
                std::string::size_type pos1=fib_entry_name.find('/',1);
                std::string::size_type pos2=fib_entry_name.find('/',pos1+1);
                std::string::size_type pos3=fib_entry_name.find('/',pos2+1);
                std::string::size_type pos4=fib_entry_name.find('/',pos3+1);
                std::string::size_type pos5=fib_entry_name.find('/',pos4+1);

                double leftdown_position_x = std::stod(fib_entry_name.substr(pos1+1,pos2-pos1-1));
                double leftdown_position_y = std::stod(fib_entry_name.substr(pos2+1,pos3-pos2-1));
                double rightup_position_x= std::stod(fib_entry_name.substr(pos3+1,pos4-pos3-1));
                double rightup_position_y= std::stod(fib_entry_name.substr(pos4+1));

//            std::cout<<"position is ("<<position_x<<","<<position_y<<")"<<std::endl;
                fib::NextHopList nexthops;
                nexthops=fib_entry_itr->getNextHops();
                fib::NextHopList::const_iterator it = nexthops.begin();
                for (;it != nexthops.end();++it) {
                    shared_ptr <Face> outFace = it->getFace();
                    gateway::Nwd::neighbors_list.insert(make_pair(gateway::Range(Coordinate(leftdown_position_x, leftdown_position_y),Coordinate(rightup_position_x,rightup_position_y)), outFace));
//                    gateway::Nwd::reverse_neighbors_list.insert(
//                            make_pair(outFace, gateway::Coordinate(position_x, position_y)));

                }
            }

        }
    }

    void
    Nwd::printNeighborsTable()
    {
        std::cout<<"-------------------------------------------------------------"<<std::endl;
        std::cout<<std::setw(35)<<"neighbor"<<std::setw(15)<<"face"<<std::endl;
        for(auto itr : gateway::Nwd::neighbors_list)
        {
            std::cout<<itr.first<<std::setw(15)<<itr.second->getRemoteUri()<<std::endl;
        }
        std::cout<<"-------------------------------------------------------------"<<std::endl;

    }

	void
	Nwd::getPointLocation(std::string interest_name,std::string& point_x,std::string& point_y)
	{
		std::string::size_type x_start=interest_name.find('/',1);
		std::string::size_type x_end = interest_name.find('/',x_start+1);
		std::string::size_type y_start=x_end;
		std::string::size_type y_end=interest_name.find('/',y_start+1);
		point_x = interest_name.substr(x_start+1,x_end-x_start-1);
		point_y = interest_name.substr(y_start+1,y_end-y_start-1);
	}

    void
    Nwd::getRangeLocation(std::string interest_name,std::string& leftdown_point_x,std::string& leftdown_point_y,std::string& rightup_point_x,std::string& rightup_point_y)
    {
        std::string::size_type left_x_start=interest_name.find('/',1);
        std::string::size_type left_x_end = interest_name.find('/',left_x_start+1);
        std::string::size_type left_y_start=left_x_end;
        std::string::size_type left_y_end=interest_name.find('/',left_y_start+1);
        leftdown_point_x = interest_name.substr(left_x_start+1,left_x_end-left_x_start-1);
        leftdown_point_y = interest_name.substr(left_y_start+1,left_y_end-left_y_start-1);

        std::string::size_type right_x_start=left_y_end;
        std::string::size_type right_x_end = interest_name.find('/',right_x_start+1);
        std::string::size_type right_y_start=right_x_end;
        std::string::size_type right_y_end = interest_name.find('/',right_y_start+1);
        rightup_point_x=interest_name.substr(right_x_start+1,right_x_end-right_x_start-1);
        rightup_point_y=interest_name.substr(right_y_start+1,right_y_end-right_y_start-1);
    }

//    void
//    Nwd::sendNdpDiscoverPacket()
//    {
//        Interest interest(Name("/ndp/discover/test"));
//        interest.setInterestLifetime(time::milliseconds(1000));
//        interest.setMustBeFresh(true);
//        // boost::this_thread::sleep(boost::posix_time::seconds(2));
//        m_face.expressInterest(interest,
//                               bind(&Nwd::onNdpData, this,  _1, _2),
//                               bind(&Nwd::onNdpTimeout, this, _1));
//
//        std::cout << "Sending " << interest << std::endl;
//    }
//
//    void
//    Nwd::onNdpData(const Interest& interest, const Data& data)
//    {
//        std::cout << data << std::endl;
//
//        std::cout << data.getContent().value()<<std::endl;
//    }
//
//    void
//    Nwd::onNdpTimeout(const Interest& interest)
//    {
//        std::cout << "Timeout " << interest << std::endl;
//    }

	void
	Nwd::WlanHotSpotStart()
	{
		std::cout<<"启动wlan热点"<<std::endl;
		system("echo 123 | sudo create_ap wlan0 eth0 ndnhotspot 12345678");
	}

	void
	Nwd::getEthernetFace()
	{
		int sockfd;
		struct ifconf ifconf;
		struct ifreq *ifreq;
		char buf[512];//缓冲区
		//初始化ifconf
		ifconf.ifc_len =512;
		ifconf.ifc_buf = buf;
		if ((sockfd =socket(AF_INET,SOCK_DGRAM,0))<0)
		{
			perror("socket" );
			exit(1);
		}
		ioctl(sockfd, SIOCGIFCONF, &ifconf); //获取所有接口信息

		//接下来一个一个的获取IP地址
		ifreq = (struct ifreq*)ifconf.ifc_buf;
//		printf("ifconf.ifc_len:%d\n",ifconf.ifc_len);
//		printf("sizeof (struct ifreq):%d\n",sizeof (struct ifreq));
        char eth_buf[16];
        char ip_buf[32];
		for (int i=(ifconf.ifc_len/sizeof (struct ifreq)); i>0; i--)
		{
			if(ifreq->ifr_flags == AF_INET){ //for ipv4
                snprintf(eth_buf,16,"%s",ifreq->ifr_name);
//				printf("name =[%s]\n" , ifreq->ifr_name);
                snprintf(ip_buf,32,"%s",inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));
//				printf("local addr = [%s]\n" ,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));
				ifreq++;
                if(strncmp(eth_buf,"eth",3) == 0 || strncmp(eth_buf,"wlan",4) == 0)
                {
                    ethface_map.insert(std::make_pair(std::string(eth_buf),std::string(ip_buf)));
                }

			}
		}
	}

	void
	Nwd::NdponInterest(const ndn::InterestFilter& filter, const Interest& interest)
	{
        Name dataName(interest.getName());
        dataName
                .append("testApp") // add "testApp" component to Interest name
                .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

        static const std::string content = "HELLO KITTY1111";

        // Create Data packet
        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

        // Sign Data packet with default identity
        m_keyChain.sign(*data);
        // m_keyChain.sign(data, <identityName>);
        // m_keyChain.sign(data, <certificate>);

        // Return Data packet to the requester
//        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
	}


    //目前为测试接口，后续添加更多功能
    void
    Nwd::nfd_location_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
    {
        std::cout << "<< I: " << interest << std::endl;
        std::string interest_name = interest.getName().toUri();

        if(interest_name.find("wsn") != std::string::npos)
        {
            std::string::size_type pos=0;
            if((pos=interest_name.find("location")) != std::string::npos)
            {
                wsn_Location_onInterest(interest_name);
            }else if(interest_name.find("topo") !=std::string::npos)
            {
                wsn_Topo_onInterest(interest_name);
            }else
            {
                wsn_onInterest(interest_name);
            }

        }else if(interest_name.find("wifi") != std::string::npos)
        {
            if(interest_name.find("location") != std::string::npos)
            {
                Wifi_Location_onInterest(interest_name);
            } else if(interest_name.find("topo") != std::string::npos)
            {
                Wifi_Topo_onInterest(interest_name);
            }

        }

//        std::string::size_type pos1;
//        if((pos1=interest_name.find("weight")) != std::string::npos)
//        {
//            std::string::size_type slash_pos1=pos1+6;
//            std::string::size_type slash_pos2=interest_name.find("/",slash_pos1+1);
//            std::string::size_type slash_pos3=interest_name.find("/",slash_pos2+1);
//            int longitude_val=std::stoi(interest_name.substr(slash_pos1+1,slash_pos2-slash_pos1-1));
//            int latitude_val=std::stoi(interest_name.substr(slash_pos2+1,slash_pos3-slash_pos2-1));
//            int weight_val=std::stoi(interest_name.substr(slash_pos3+1));
//            std::pair<int,int> route_weight_key(longitude_val,latitude_val);
//
//            routeweight_map[route_weight_key]=weight_val;   //更新权值
//
//        }
//        std::cout << "<< I: " << interest << std::endl;

        // Create new name, based on Interest's name
//        Name dataName(interest.getName());
//        dataName
//                .append("testApp") // add "testApp" component to Interest name
//                .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)
//
//        static const std::string content = "HELLO KITTY1111";
//
//        // Create Data packet
//        shared_ptr<Data> data = make_shared<Data>();
//        data->setName(dataName);
//        data->setFreshnessPeriod(time::seconds(10));
//        data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//
//        // Sign Data packet with default identity
//        m_keyChain.sign(*data);
//        // m_keyChain.sign(data, <identityName>);
//        // m_keyChain.sign(data, <certificate>);
//
//        // Return Data packet to the requester
////        std::cout << ">> D: " << *data << std::endl;
//        m_face.put(*data);

    }

    void
    Nwd::NDNIOT_location_onInterest(std::string& interest_name,const std::function<void(const std::shared_ptr<Face>& )>& callback ) {


        if (interest_name.find("wsn") != std::string::npos) {
            std::string::size_type pos = 0;
            if ((pos = interest_name.find("location")) != std::string::npos) {
                wsn_Location_onInterest(interest_name);
            } else if (interest_name.find("topo") != std::string::npos) {
                wsn_Topo_onInterest(interest_name);
            } else {
                wsn_onInterest(interest_name);

//                Name dataName(interest_name);
//
//                static const std::string content = "HELLO KITTY1111";
//
//                shared_ptr<Data> data = make_shared<Data>();
//                data->setName(dataName);
//                data->setFreshnessPeriod(time::seconds(10));
//                data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//                m_keyChain.sign(*data);
//
//                std::cout << ">> D: " << *data << std::endl;
//                m_face.put(*data);
            }

        } else if (interest_name.find("wifi") != std::string::npos) {
            if (interest_name.find("location") != std::string::npos) {
                Wifi_Location_onInterest(interest_name);
            } else if (interest_name.find("topo") != std::string::npos) {
                Wifi_Topo_onInterest(interest_name);
            } else{
                Wifi_onInterest(interest_name,callback);

//                Name dataName(interest_name);
//
//                static const std::string content = "HELLO KITTY1111";
//
//                shared_ptr<Data> data = make_shared<Data>();
//                data->setName(dataName);
//                data->setFreshnessPeriod(time::seconds(10));
//                data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//                m_keyChain.sign(*data);
//
//                std::cout << ">> D: " << *data << std::endl;
//                m_face.put(*data);
            }

        }
    }

    void
    Nwd::Wifi_onInterest(std::string& interest_name,const std::function<void(const std::shared_ptr<Face>& )>& callback)
    {
        auto fib_entry_itr=m_forwarder->getFib().begin();

        for(;fib_entry_itr!=m_forwarder->getFib().end();fib_entry_itr++){
            std::ostringstream os;
            os<<(*fib_entry_itr).getPrefix()<<std::endl;
            std::string fib_entry_name=os.str();
            /*查找最近路由接口并转发，目前路由前缀为/nfd,后接地理位置*/
            if(fib_entry_name.find("/NDN-WIFI")!=std::string::npos ) {
                std::cout<<"fib name is: "<<fib_entry_name;


//            std::cout<<"position is ("<<position_x<<","<<position_y<<")"<<std::endl;
                fib::NextHopList nexthops;
                nexthops=fib_entry_itr->getNextHops();
                fib::NextHopList::const_iterator it = nexthops.begin();
                for (;it != nexthops.end();++it) {
                    shared_ptr <Face> outFace = it->getFace();
                    callback(outFace);
//                    gateway::Nwd::neighbors_list.insert(make_pair(gateway::Range(Coordinate(leftdown_position_x, leftdown_position_y),Coordinate(rightup_position_x,rightup_position_y)), outFace));
//                    gateway::Nwd::reverse_neighbors_list.insert(
//                            make_pair(outFace, gateway::Coordinate(position_x, position_y)));

                }
            }

        }


//        Name dataName(interest_name);
//        std::string data_val("test");
//        shared_ptr<Data> data = make_shared<Data>();
//        data->setName(dataName);
//        data->setFreshnessPeriod(time::seconds(10));
//        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
//        m_keyChain.sign(*data);
//
//        std::cout << ">> D: " << *data << std::endl;
//        m_face.put(*data);
    }

    /*目前没有使用*/
	void
	Nwd::Wsn_Range_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		int max_x=0,max_y=0,min_x=INT_MAX,min_y=INT_MAX;
		for(auto& itr:wsn_location_map){
//			std::cout<<itr.second<<std::endl;
			std::string::size_type x_start=itr.second.find("(");
			std::string::size_type x_end=itr.second.find(",");
			std::string::size_type y_start=x_end;
			std::string::size_type y_end= itr.second.find(")");
			std::string x=itr.second.substr(x_start+1,x_end-x_start-1);
			std::string y=itr.second.substr(y_start+1,y_end-y_start-1);
			std::cout<<x<<","<<y<<std::endl;
			int x_int=std::stoi(x);
			int y_int=std::stoi(y);
			if(x_int<min_x)
				min_x=x_int;
			if(x_int>max_x)
				max_x=x_int;
			if(y_int<min_y)
				min_y=y_int;
			if(y_int>max_y)
				max_y=y_int;
		
		}
		wsn_nodes=wsn_location_map.size();
		Name dataName(interest_name);
	  	std::string data_val("("+std::to_string(min_x)+","+std::to_string(max_y)+")/("+
			std::to_string(max_x)+","+std::to_string(min_y)+") "+to_string(wsn_nodes)+"$");
	  	shared_ptr<Data> data = make_shared<Data>();
     	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  	m_keyChain.sign(*data);

	    std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
	}

    /*保存更新wifi用户id和地理位置  */
    void
    Nwd::wifi_update_id_location(std::string& str,std::string& user_id,std::string& location)
    {
        std::string::size_type slash=str.find('/',1);

        user_id=str.substr(1,slash-1);
        location=str.substr(slash+1);
        slash=location.find('/');
        location.replace(slash,1,",");
        std::cout<<user_id<<",  "<<location<<std::endl;
        if(wifi_location_map.find(user_id) == wifi_location_map.end())
            wifi_location_map[user_id] = std::make_pair(wifi_num++,location);
    }

    void
    Nwd::Wifi_Location_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
    {
        std::cout << "<< I: " << interest << std::endl;
        std::string interest_name = interest.getName().toUri();
        std::string data_val;
        for(auto& itr : wifi_location_map)
        {
            data_val.append(std::to_string(itr.second.first)+"->(");
            data_val.append(itr.second.second);
            data_val.append(")$$");

        }
        std::cout<<data_val<<std::endl;
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
    }

    void
    Nwd::Wifi_Location_onInterest(std::string& interest_name)
    {

        std::string data_val;
        for(auto& itr : wifi_location_map)
        {
            data_val.append(std::to_string(itr.second.first)+"->(");
            data_val.append(itr.second.second);
            data_val.append(")$$");

        }
        std::cout<<data_val<<std::endl;
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
    }

    void
    Nwd::Wifi_Topo_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
    {
        std::cout << "<< I: " << interest << std::endl;
        std::string interest_name = interest.getName().toUri();
        std::string data_val;
        for(auto& itr : wifi_location_map)
        {
            data_val.append(itr.first+"->0$$");
        }
        std::cout<<data_val<<std::endl;
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
    }

    void
    Nwd::Wifi_Topo_onInterest(std::string interest_name)
    {

        std::string data_val;
        for(auto& itr : wifi_location_map)
        {
            data_val.append(std::to_string(itr.second.first)+"->0$$");
        }
        std::cout<<data_val<<std::endl;
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);
    }

	void
	Nwd::Wifi_Register_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();

		std::string tmp=interest_name;
		tmp.erase(0,22);
		std::cout<<tmp<<std::endl;
        std::string user_id;
        std::string location;
        wifi_update_id_location(tmp,user_id,location);

		auto pit_entry_itr=m_forwarder->getPit().begin();
//		++pit_entry_itr;
		for(;pit_entry_itr!=m_forwarder->getPit().end();pit_entry_itr++){

			std::ostringstream os;
			os<<(*pit_entry_itr).getInterest()<<std::endl;
			if(os.str().find("/NDN-IOT-REGISTER")!=std::string::npos){
//				std::cout<<os.str()<<std::endl;
				for(auto pit_entryIn_itr=(*pit_entry_itr).in_begin();pit_entryIn_itr!=(*pit_entry_itr).in_end();pit_entryIn_itr++){
//					std::cout<<(*pit_entryIn_itr).getInterest()<<std::endl;
					std::ostringstream oss;
//					std::cout<<(*pit_entryIn_itr).getFace()->getLocalUri()<<std::endl;
					oss<<(*pit_entryIn_itr).getFace()->getRemoteUri()<<std::endl;
					std::string remote_name=oss.str();
					std::string::size_type pos=remote_name.rfind(":");
					remote_name.erase(pos);
					std::cout<<remote_name<<std::endl;
                    ribRegisterPrefix("/NDN-WIFI/"+user_id,remote_name);
//					std::cout<<"face name:"<<face_name<<std::endl;
//					ribRegisterPrefix();


				}
			}
		}
//		boost::this_thread::sleep(boost::posix_time::seconds(2)); 
		

		Name dataName(interest_name);
	  	std::string data_val("success/"+gateway_area);
	  	shared_ptr<Data> data = make_shared<Data>();
     	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
	  	m_keyChain.sign(*data);
	
	    std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);

        //广播发送
//		face_name="/wifi";
//		std::cout<<"face name:"<<face_name<<std::endl;
//		ribRegisterPrefix();
//        strategyChoiceSet(face_name,"ndn:/localhost/nfd/strategy/broadcast");

//		nfd::FaceTable& faceTable = m_forwarder->getFaceTable();
//		for(auto itr=faceTable.begin();itr!=faceTable.end();itr++){
//			std::cout<<(*itr)->getRemoteUri()<<std::endl;
//		}
//		for(auto itr:faceTable.m_faces){
//			std::cout<<*itr.first<<std::endl;
//		}
//		m_forwarder->getFaceTable();

	}

    void
    Nwd::strategyChoiceSet(std::string name,std::string strategy)
    {


        ControlParameters parameters;
        parameters
                .setName(name)
                .setStrategy(strategy);

        m_controller.start<StrategyChoiceSetCommand>(parameters,
                                                     bind(&Nwd::onSuccess, this, _1,
                                                          "Successfully set strategy choice"),
                                                     bind(&Nwd::onError, this, _1, _2,
                                                          "Failed to set strategy choice"));
        std::cout<<"strategyset: "<<name<<" ";
    }



	void
	Nwd::ribRegisterPrefix(std::string face_name,std::string faceName)
	{
//		m_name=face_name;
//		const std::string& faceName=remote_name;
		FaceIdFetcher::start(m_face, m_controller, faceName, true,
                       [this,face_name] (const uint32_t faceId) {
                         ControlParameters parameters;
                         parameters
                           .setName(face_name)
                           .setCost(m_cost)
                           .setFlags(m_flags)
                           .setOrigin(m_origin)
                           .setFaceId(faceId);

                         if (m_expires != DEFAULT_EXPIRATION_PERIOD)
                           parameters.setExpirationPeriod(m_expires);

                         m_controller
                           .start<RibRegisterCommand>(parameters,
                                                      bind(&Nwd::onSuccess, this, _1,
                                                           "Successful in name registration"),
                                                      bind(&Nwd::onError, this, _1, _2,
                                                           "Failed in name registration"));
                       },
                       bind(&Nwd::onObtainFaceIdFailure, this, _1));
	}

    void
    Nwd::ribUnregisterPrefix(std::string m_name,std::string faceName)
    {

        FaceIdFetcher::start(m_face, m_controller, faceName, false,
                             [this,m_name] (const uint32_t faceId) {
                                 ControlParameters parameters;
                                 parameters
                                         .setName(m_name)
                                         .setFaceId(faceId)
                                         .setOrigin(m_origin);

                                 m_controller
                                         .start<RibUnregisterCommand>(parameters,
                                                                      bind(&Nwd::onSuccess, this, _1,
                                                                           "Successful in unregistering name"),
                                                                      bind(&Nwd::onError, this, _1, _2,
                                                                           "Failed in unregistering name"));
                             },
                             bind(&Nwd::onObtainFaceIdFailure, this, _1));
    }

	void
	Nwd::time_sync_init(){
		m_tsync.expires_from_now(std::chrono::seconds(120));
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
	}

	void
	Nwd::time_sync(){
        if(time_sync_count<5){
            if(local_timestamp % (10) ==0){
                local_timestamp=0;
                std::cout<<"时间同步"<<std::endl;
                m_serialManager.sync_time(local_timestamp);
                std::time(&globe_timestamp);
                ++time_sync_count;
            }
        }else{
            if(local_timestamp % (3000) ==0){
                local_timestamp=0;
                std::cout<<"时间同步"<<std::endl;
                m_serialManager.sync_time(local_timestamp);
                std::time(&globe_timestamp);
            }
        }

		
//		
		++local_timestamp;
//		std::cout<<"time sync:"<<globe_timestamp+local_timestamp<<std::endl;

		m_tsync.expires_from_now(std::chrono::seconds(1));//set 1s timer
		m_tsync.async_wait(boost::bind(&Nwd::time_sync,this));
	}

	void
	Nwd::Location_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;
		for(auto& itr:wsn_location_map){
			data_val+=itr.first;
			data_val+="->";
			data_val+=itr.second;
			data_val+="$$";
		}
	    Name dataName(interest_name);
	  
	   	shared_ptr<Data> data = make_shared<Data>();
      	data->setName(dataName);
      	data->setFreshnessPeriod(time::seconds(10));
      	data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	 	 m_keyChain.sign(*data);

	  	std::cout << ">> D: " << *data << std::endl;
      	m_face.put(*data);
		
	}

    void
    Nwd::wsn_Location_onInterest(std::string& interest_name)
    {

        std::string data_val;
        for(auto& itr:wsn_location_map){
            data_val+=itr.first;
            data_val+="->";
            data_val+=itr.second;
            data_val+="$$";
        }
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);

    }

	void
	Nwd::Topo_onInterest(const ndn::InterestFilter& filter, const Interest& interest)
	{
		std::cout << "<< I: " << interest << std::endl;
		std::string interest_name = interest.getName().toUri();
		std::string data_val;	
//		std::cout<<"before"<<std::endl;
		for(auto & itr:topo_data){
			data_val+=itr;
			std::cout<<itr<<std::endl;
			data_val+="$$";
		}
		Name dataName(interest_name);
	  
	  shared_ptr<Data> data = make_shared<Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(time::seconds(10));
      data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
	  m_keyChain.sign(*data);

	  std::cout << ">> D: " << *data << std::endl;
      m_face.put(*data);
		
	}

    void
    Nwd::wsn_Topo_onInterest(std::string& interest_name)
    {

        std::string data_val;
//		std::cout<<"before"<<std::endl;
        for(auto & itr:topo_data){
            data_val+=itr;
            std::cout<<itr<<std::endl;
            data_val+="$$";
        }
        Name dataName(interest_name);

        shared_ptr<Data> data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(time::seconds(10));
        data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()),data_val.size());
        m_keyChain.sign(*data);

        std::cout << ">> D: " << *data << std::endl;
        m_face.put(*data);

    }

    void
  	Nwd::onInterest(const ndn::InterestFilter& filter, const Interest& interest)
  	{
    	
		std::string interest_name = interest.getName().toUri();
		if(interest_name.find("topo") != std::string::npos)
			return ;
		if(interest_name.find("location") != std::string::npos)
			return ;
		if(interest_name.find("range") != std::string::npos)
			return ;
		std::cout << "<< I: " << interest << std::endl;
		
		std::string::size_type pos = 0;
        while ((pos = interest_name.find("%2C", pos)) != std::string::npos){
            interest_name.replace(pos, 3, ",");
            pos += 1;
        }
		interest_name.replace(0, 1, "");
//		std::string interest_copy=interest_name;
		interest_list.insert(std::make_pair(interest_name,std::set<std::string>()));

		receive_in_queue.push(interest_name);
		std::cout<<interest_name<<std::endl;
		
		
		std::time(&globe_timestamp);
		std::cout<<"time:"<<globe_timestamp<<std::endl;
		if(m_serialManager.time_belong_interest(interest_name,globe_timestamp)){			
			if(!handle_interest_busy){
				handle_interest_busy=true;
				while(!receive_in_queue.empty()){
					std::string recv_in=receive_in_queue.front();
				
					char tmp[1024];
        			strcpy(tmp, recv_in.c_str());
					m_serialManager.handle_interest(tmp);
					receive_in_queue.pop();
					boost::this_thread::sleep(boost::posix_time::milliseconds(500));
				}
				handle_interest_busy=false;
				m_t.expires_from_now(std::chrono::milliseconds(1600));//set 400ms timer
				
				m_t.async_wait(boost::bind(&Nwd::wait_data));
			}
		}else{
			search_dataset(interest_name);
		}
  	}

    void
    Nwd::getRangeLocation_Time_DataType(std::string interest_name,std::string& leftdown_point_x,std::string& leftdown_point_y,std::string& rightup_point_x,std::string& rightup_point_y,std::string& start_time,std::string& end_time,std::string& datatype, std::string& space_time)
    {
        std::string::size_type left_x_start=interest_name.find('/',1);
        std::string::size_type left_x_end = interest_name.find('/',left_x_start+1);
        std::string::size_type left_y_start=left_x_end;
        std::string::size_type left_y_end=interest_name.find('/',left_y_start+1);
        leftdown_point_x = interest_name.substr(left_x_start+1,left_x_end-left_x_start-1);
        leftdown_point_y = interest_name.substr(left_y_start+1,left_y_end-left_y_start-1);

        std::string::size_type right_x_start=left_y_end;
        std::string::size_type right_x_end = interest_name.find('/',right_x_start+1);
        std::string::size_type right_y_start=right_x_end;
        std::string::size_type right_y_end = interest_name.find('/',right_y_start+1);
        std::string::size_type start_time_begin=right_y_end;
        std::string::size_type start_time_end = interest_name.find('/',start_time_begin+1);
        std::string::size_type end_time_begin=start_time_end;
        std::string::size_type end_time_end = interest_name.find('/',end_time_begin+1);
        std::string::size_type data_type_start = end_time_end;
        std::string::size_type data_type_end = interest_name.find('/',data_type_start+1);
        std::string::size_type netid_start=data_type_end;
        std::string::size_type netid_end=interest_name.find('/',netid_start+1);
        std::string::size_type space_time_start = netid_end;
        std::string::size_type space_time_end = interest_name.find('/',space_time_start+1);
        rightup_point_x=interest_name.substr(right_x_start+1,right_x_end-right_x_start-1);
        rightup_point_y=interest_name.substr(right_y_start+1,right_y_end-right_y_start-1);
        start_time=interest_name.substr(start_time_begin+1,start_time_end-start_time_begin-1);
        end_time=interest_name.substr(end_time_begin+1,end_time_end-end_time_begin-1);
        datatype=interest_name.substr(data_type_start+1,data_type_end-data_type_start-1);
        space_time=interest_name.substr(space_time_start+1,space_time_end-space_time_start-1);
    }


    void
    Nwd::wsn_splite_location(std::string& str)
    {
        std::string::size_type pos= str.find(".");
        str=str.substr(pos+3,4);
    }

    void
    Nwd::wsn_onInterest(std::string interest_name)
    {
        origin_wsn_interestname.push(interest_name);
        std::string leftdown_point_x, leftdown_point_y;
        std::string rightup_point_x,rightup_point_y;
        std::string start_time,end_time;
        std::string datatype;
        std::string space_time;
        getRangeLocation_Time_DataType(interest_name, leftdown_point_x, leftdown_point_y, rightup_point_x,rightup_point_y,start_time,end_time,datatype,space_time);
        std::cout<<"space_time is "<<space_time<<std::endl;
        std::cout<<"data type is :"<<datatype<<std::endl;
        wsn_splite_location(leftdown_point_x);
        wsn_splite_location(leftdown_point_y);
        wsn_splite_location(rightup_point_x);
        wsn_splite_location(rightup_point_y);


        interest_name="wsn/"+leftdown_point_x+","+rightup_point_y+"/"+rightup_point_x+","+leftdown_point_y+"/"+start_time+"/"+end_time+"/"+datatype+"/"+space_time;  //转化成左下右上
//		std::string interest_copy=interest_name;
        interest_list.insert(std::make_pair(interest_name,std::set<std::string>()));

        receive_in_queue.push(interest_name);
        std::cout<<interest_name<<std::endl;


        std::time(&globe_timestamp);
        std::cout<<"time:"<<globe_timestamp<<std::endl;
        if(m_serialManager.time_belong_interest(interest_name,globe_timestamp)){
            if(!handle_interest_busy){
                handle_interest_busy=true;
                while(!receive_in_queue.empty()){
                    std::string recv_in=receive_in_queue.front();

                    char tmp[1024];
                    strcpy(tmp, recv_in.c_str());
                    std::cout<<"begin handle interest"<<std::endl;
                    m_serialManager.handle_interest(tmp);
                    receive_in_queue.pop();
//                    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                }
                handle_interest_busy=false;
                std::shared_ptr<boost::asio::steady_timer> timer_ptr(new boost::asio::steady_timer(getGlobalIoService()));
                timer_ptr->expires_from_now(std::chrono::milliseconds(1000));
                timer_ptr->async_wait(boost::bind(&Nwd::wait_data));
                timer_queue.push(timer_ptr);
//                m_t.expires_from_now(std::chrono::milliseconds(2600));//set 400ms timer
//
//                m_t.async_wait(boost::bind(&Nwd::wait_data));
            }
        }else{
            search_dataset(interest_name);
        }
    }

  void
  Nwd::search_dataset(std::string In_Name){
  		std::string data_ret;
//        std::cout<<"data_set size is"<<data_ret.size()<<std::endl;
		for(auto &itr:data_set){
			if(search_dataset_position(In_Name,itr)){
				if(search_dataset_time(In_Name,itr)){
					if(search_dataset_type(In_Name,itr)){
//						std::cout<<"data_set match"<<std::endl;
						data_ret+=itr.data;
						data_ret+="$$";
					}
				}
			}
		}
        std::cout<<"data is :"<<data_ret<<std::endl;
        send_data(In_Name,data_ret);
  }

  bool
  Nwd::search_dataset_type(std::string In_Name,WsnData dataval){
      std::string::size_type end_pos=In_Name.rfind('/');
      std::string::size_type start_pos=In_Name.rfind('/',end_pos-1);
      std::string data_type=In_Name.substr(start_pos+1,end_pos-start_pos-1);
//	std::cout<<In_Name<<std::endl;
//	std::cout<<data_type<<std::endl;
	if(data_type == dataval.type){
//		std::cout<<"dataset type match"<<std::endl;
		return true;
	}
	return false;

  }
	
  bool
  Nwd::search_dataset_time(std::string In_Name,WsnData dataval){
    std::string::size_type in_time_beg,in_time_mid,in_time_end;
//    std::cout<<"In_name : "<<In_Name<<std::endl;
    in_time_end=In_Name.rfind('/');
    in_time_end=In_Name.rfind('/',in_time_end-1);
	in_time_mid=In_Name.rfind('/',in_time_end-1);
	in_time_beg=In_Name.rfind('/',in_time_mid-1);

	int In_Start_Time=std::stoi(In_Name.substr(in_time_beg+1,in_time_mid-in_time_beg-1));
	int In_End_Time=std::stoi(In_Name.substr(in_time_mid+1,in_time_end-in_time_mid-1));

	if(dataval.wsn_time>=In_Start_Time && dataval.wsn_time<=In_End_Time){
//		std::cout<<"dataset time match"<<std::endl;
		return true;
	}
	return false;


  }

  bool
  Nwd::search_dataset_position(std::string Interest,WsnData dataval){
		std::string::size_type in_scope_beg,in_scope_mid,in_scope_end;
//		std::string::size_type data_scope_beg,data_scope_end;
		in_scope_beg=Interest.find('/');
		in_scope_mid=Interest.find('/',in_scope_beg+1);
		in_scope_end=Interest.find('/',in_scope_mid+1);


		std::string in_scope_leftup=Interest.substr(in_scope_beg+1,in_scope_mid-in_scope_beg-1);
		std::string in_scope_rightdown=Interest.substr(in_scope_mid+1,in_scope_end-in_scope_mid-1);

		int in_leftup_x=std::stoi(in_scope_leftup.substr(0,in_scope_leftup.find(',')));
		int in_leftup_y=std::stoi(in_scope_leftup.substr(in_scope_leftup.find(',')+1));
		int in_rightdown_x=std::stoi(in_scope_rightdown.substr(0,in_scope_rightdown.find(',')));
		int in_rightdown_y=std::stoi(in_scope_rightdown.substr(in_scope_rightdown.find(',')+1));

//        std::cout<<"In :" <<in_leftup_x <<" "<<in_leftup_y <<" "<<in_rightdown_x <<" "<<in_rightdown_y<<std::endl;
//        std::cout<<"dataval: "<<dataval.position_x<<" "<<dataval.position_y<<std::endl;
        if(dataval.position_x ==0 || dataval.position_y == 0)
            return  true;

		if(dataval.position_x>=in_leftup_x && dataval.position_x<=in_rightdown_x){
			if(dataval.position_y>=in_rightdown_y && dataval.position_y<=in_leftup_y){			
//				std::cout<<"data_set position match"<<std::endl;
				return true;
			}	
		}
		return false;
  }

  void
  Nwd::onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cout << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();

  }

  void 
  Nwd::listen_wsn_data(serial_manager *sm)
  {    
  	   sm->read_data(interest_list,wsn_location_map);

  }

  void
  Nwd::wait_data(){

		for(auto &itr:interest_list){
		    std::string data_ret;
		    std::string In_Name;
			In_Name=itr.first;
			for(auto &data:itr.second){
//				std::cout<<data<<std::endl;
//				data_ret+=data;
				data_set.insert(WsnData(data,local_timestamp+globe_timestamp)); //put in data_set
//				data_ret+="$$";
			}
			search_dataset(In_Name);
//			if(!data_ret.empty()){
//				
//				send_data(In_Name,data_ret);
//			}
		}
      timer_queue.pop();
//		for(auto &itr:data_set)
//			std::cout<<itr<<std::endl;
		
  }
  
  void
  Nwd::send_data(std::string In_Name,std::string data_val)
  {
      std::string last_name; //记录上一个兴趣包的名字
      while(!origin_wsn_interestname.empty()){
          std::string origin_name=origin_wsn_interestname.front();//转化为新的数据命名方式，使用队列来保存的兴趣包名字，现在只是先进先出，以后做数据包名字的转换
          if(last_name =="")
              last_name=origin_name;
          else if(last_name != origin_name)
          {
              break;
          }
          else{
                origin_wsn_interestname.pop();
                break;
          }


          origin_wsn_interestname.pop();
          Name dataName(origin_name);

          shared_ptr<Data> data = make_shared<Data>();
          data->setName(dataName);
          data->setFreshnessPeriod(time::seconds(10));
          data->setContent(reinterpret_cast<const uint8_t*>(data_val.c_str()), data_val.size());
          m_keyChain.sign(*data);

          std::cout << ">> D: " << *data << std::endl;
          m_face.put(*data);
          interest_list.erase(In_Name); //delete the Interest stored in interest_list
      }

  }

  void 
  Nwd::manage_wsn_topo(serial_manager *sm)
  {
	sm->topo_management(topo_data);
  }

  void
  Nwd::onSuccess(const ControlParameters& commandSuccessResult, const std::string& message)
  {
	 std::cout << message << ": " << commandSuccessResult << std::endl;
  }

  void
  Nwd::onError(uint32_t code, const std::string& error, const std::string& message)
  {
     std::ostringstream os;
     os << message << ": " << error << " (code: " << code << ")";
     BOOST_THROW_EXCEPTION(Error(os.str()));
  }

  void
  Nwd::onObtainFaceIdFailure(const std::string& message)
 {
 	 BOOST_THROW_EXCEPTION(Error(message));
 }
  

  Nwd::FaceIdFetcher::FaceIdFetcher(ndn::Face& face,
                                   ndn::nfd::Controller& controller,
                                   bool allowCreate,
                                   const SuccessCallback& onSucceed,
                                   const FailureCallback& onFail)
  : m_face(face)
  , m_controller(controller)
  , m_allowCreate(allowCreate)
  , m_onSucceed(onSucceed)
  , m_onFail(onFail){}

 void
 Nwd::FaceIdFetcher::start(ndn::Face& face,
                           ndn::nfd::Controller& controller,
                           const std::string& input,
                           bool allowCreate,
                           const SuccessCallback& onSucceed,
                           const FailureCallback& onFail)
{
  // 1. Try parse input as FaceId, if input is FaceId, succeed with parsed FaceId
  // 2. Try parse input as FaceUri, if input is not FaceUri, fail
  // 3. Canonize faceUri
  // 4. If canonization fails, fail
  // 5. Query for face
  // 6. If query succeeds and finds a face, succeed with found FaceId
  // 7. Create face
  // 8. If face creation succeeds, succeed with created FaceId
  // 9. Fail
  
  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(input, e)) {
    try
      {
        u_int32_t faceId = boost::lexical_cast<uint32_t>(input);
		
        onSucceed(faceId);
        return;
      }
    catch (boost::bad_lexical_cast&)
      {
      	
        onFail("No valid faceId or faceUri is provided");
        return;
      }
  }
  else {
  	
    ndn::util::FaceUri faceUri;
    if (!faceUri.parse(input)) {
      onFail("FaceUri parse failed");
      return;
    }
	
    auto fetcher = new FaceIdFetcher(std::ref(face), std::ref(controller),
                                     allowCreate, onSucceed, onFail);

    fetcher->startGetFaceId(faceUri);
  }
}

void
Nwd::FaceIdFetcher::startGetFaceId(const ndn::util::FaceUri& faceUri)
{
 
  faceUri.canonize(bind(&FaceIdFetcher::onCanonizeSuccess, this, _1),
                   bind(&FaceIdFetcher::onCanonizeFailure, this, _1),
                   m_face.getIoService(), ndn::time::seconds(4));
}

void
Nwd::FaceIdFetcher::onCanonizeSuccess(const ndn::util::FaceUri& canonicalUri)
{
  
  ndn::Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceQueryFilter queryFilter;
  queryFilter.setRemoteUri(canonicalUri.toString());
  queryName.append(queryFilter.wireEncode());

  ndn::Interest interestPacket(queryName);
  interestPacket.setMustBeFresh(true);
  interestPacket.setInterestLifetime(ndn::time::milliseconds(4000));
  auto interest = std::make_shared<ndn::Interest>(interestPacket);
  
  ndn::util::SegmentFetcher::fetch(m_face, *interest,
                                   m_validator,
                                   bind(&FaceIdFetcher::onQuerySuccess,
                                        this, _1, canonicalUri),
                                   bind(&FaceIdFetcher::onQueryFailure,
                                        this, _1, canonicalUri));
} 

 void
 Nwd::FaceIdFetcher::onCanonizeFailure(const std::string& reason)
 {
	fail("Canonize faceUri failed : " + reason);
 }

 void
 Nwd::FaceIdFetcher::onQuerySuccess(const ndn::ConstBufferPtr& data,
                                    const ndn::util::FaceUri& canonicalUri)
{
  
  size_t offset = 0;
  bool isOk = false;
  ndn::Block block;
  std::tie(isOk, block) = ndn::Block::fromBuffer(data, offset);

  if (!isOk) {
    if (m_allowCreate) {
      startFaceCreate(canonicalUri);
    }
    else {
      fail("Fail to find faceId");
    }
  }
  else {
    try {
      FaceStatus status(block);
      succeed(status.getFaceId());
    }
    catch (const ndn::tlv::Error& e) {
      std::string errorMessage(e.what());
      fail("ERROR: " + errorMessage);
    }
  }
}

void
Nwd::FaceIdFetcher::onQueryFailure(uint32_t errorCode,
                                    const ndn::util::FaceUri& canonicalUri)
{
  std::stringstream ss;
  ss << "Cannot fetch data (code " << errorCode << ")";
  fail(ss.str());
}

void
Nwd::FaceIdFetcher::fail(const std::string& reason)
{
	m_onFail(reason);
	delete this;
}

void
Nwd::FaceIdFetcher::succeed(uint32_t faceId)
{
  
  m_onSucceed(faceId);
  
  delete this;
}

  void
  Nwd::FaceIdFetcher::startFaceCreate(const ndn::util::FaceUri& canonicalUri)
  {
	 ControlParameters parameters;
 	 parameters.setUri(canonicalUri.toString());

  	 m_controller.start<FaceCreateCommand>(parameters,
                                        [this] (const ControlParameters& result) {
                                          succeed(result.getFaceId());
                                        },
                                        bind(&FaceIdFetcher::onFaceCreateError, this, _1, _2,
                                             "Face creation failed"));
  }

 void
 Nwd::FaceIdFetcher::onFaceCreateError(uint32_t code,
                                       const std::string& error,
                                       const std::string& message)
 {
  	std::stringstream ss;
  	ss << message << " : " << error << " (code " << code << ")";
 	fail(ss.str());
 }



  }
}
