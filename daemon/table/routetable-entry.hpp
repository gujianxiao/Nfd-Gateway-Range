//
// Created by youngwb on 3/25/17.
//

#ifndef NFD_MASTER_ROUTETABLE_ENTRY_HPP
#define NFD_MASTER_ROUTETABLE_ENTRY_HPP

#include "../coordinate.h"
#include "../range.hpp"
#include "../face/face.hpp"

namespace nfd{
    namespace face{
        class Face;
    }
    namespace gateway{
    class RouteTableEntry
    {
    public:
        enum Reach_Status {neighbor,unreachable,reachable,unknown,minlocal};
        enum Send_Status {flood,sending,notsending,received};
    private:
//        gateway::Coordinate dest_;
        gateway::Range nexthop_;
        double weight_;
        Reach_Status rs_flag_;
        Send_Status ss_flag_;
        std::shared_ptr<Face> outface;
    public:

        RouteTableEntry(const Range &nexthop, double weight, Reach_Status rs_flag,Send_Status ss_flag, std::shared_ptr<Face>& face):
            nexthop_(nexthop),weight_(weight),rs_flag_(rs_flag),ss_flag_(ss_flag),outface(face){}

//        RouteTableEntry():nexthop_(),weight_(0),rs_flag_(unknown),ss_flag_(notsending){}

//        gateway::Coordinate get_dest() const
//        {
//            return  dest_;
//        }


        gateway::Range get_nexthop() const
        {
            return nexthop_;
        }

        double get_weight() const
        {
            return weight_;
        }

        Reach_Status  get_reachstatus() const
        {
            return rs_flag_;
        }

        Send_Status get_sendstatus() const
        {
            return ss_flag_;
        }

       std::shared_ptr<Face> get_face() const
       {
           return  outface;
       }


        void set_reachstatus(Reach_Status s)
        {
            rs_flag_=s;
        }

        void set_sendstatus(Send_Status s)
        {
            ss_flag_=s;
        }
    };

    std::ostream& operator<< (std::ostream& os ,  const RouteTableEntry & re);
    }
}
#endif //NFD_MASTER_ROUTETABLE_ENTRY_HPP
