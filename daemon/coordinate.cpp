//
// Created by youngwb on 3/23/17.
//

#include "coordinate.h"
#include <iomanip>

namespace nfd {
    namespace gateway {

    double distance(const Coordinate& c1, const Coordinate& c2)
    {
	//std::cout<<"c1 longitude  "<<std::stod(std::to_string(c1.get_longitude()*1000000))<<std::endl;
        long double ret=static_cast<long double>(abs(c1.get_longitude()*1000000-c2.get_longitude()*1000000))*static_cast<long double>(abs(c1.get_longitude()*1000000-c2.get_longitude()*1000000))+static_cast<long double>(abs(c1.get_latitude()*1000000-c2.get_latitude()*1000000))*static_cast<long double>(abs(c1.get_latitude()*1000000-c2.get_latitude()*1000000));
	//std::cout<<"distance is "<<ret<<"  "<<static_cast<double>(sqrt(ret))<<std::endl;
        return static_cast<double>(sqrt(ret));
    }

    std::ostream& operator << (std::ostream& output,const Coordinate& c)
    {
        output<<"("<<std::setw(10)<<std::internal<<std::to_string(c.get_longitude())<<","<<std::setw(10)<<std::internal<<std::to_string(c.get_latitude())<<")";
        return  output;
    }


    }
}
