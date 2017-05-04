//
// Created by youngwb on 5/4/17.
//
#include "range.hpp"

namespace nfd {
    namespace gateway {

        double rangedistance(const Range& r1, const Range& r2)
        {
            Coordinate range1_coordinate((r1.rightup_.get_longitude()+r1.leftdown_.get_longitude())/2,(r1.rightup_.get_latitude()+r1.leftdown_.get_latitude())/2);
            Coordinate range2_coordinate((r2.rightup_.get_longitude()+r2.leftdown_.get_longitude())/2,(r2.rightup_.get_latitude()+r2.leftdown_.get_latitude())/2);
//            std::cout<<range1_coordinate<< " " <<range2_coordinate<<std::endl;
            return distance(range1_coordinate,range2_coordinate);  //调用点间距离的计算
        }

        std::ostream& operator << (std::ostream& output,const Range& c)
        {
//            output<<"("<<std::setw(6)<<std::internal<<c.get_longitude()<<" , "<<std::setw(6)<<std::internal<<c.get_latitude()<<")";
            output<<c.leftdown_<<c.rightup_;
            return  output;
        }


    }
}