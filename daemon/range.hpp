//
// Created by youngwb on 5/4/17.
//

#ifndef NFD_MASTER_RANGE_RANGE_HPP
#define NFD_MASTER_RANGE_RANGE_HPP

#include "coordinate.h"
namespace nfd {
    namespace gateway {

        class Coordinate;
        class CoordinateHash;
        class CoordinateEqual;

        class Range {
        private:
            Coordinate leftdown_;
            Coordinate rightup_;
        public:
            Range(Coordinate leftdown,Coordinate rightup):leftdown_(leftdown),rightup_(rightup){}
            Range():leftdown_(),rightup_(){}

            Coordinate get_leftdown() const
            {
                return  leftdown_;
            }

            Coordinate get_rightup() const
            {
                return rightup_;
            }

            void set_leftdown(const Coordinate c)
            {
                leftdown_=c;
            }

            void  set_rightup(const Coordinate c)
            {
                rightup_=c;
            }



            friend  class  RangeHash;
            friend  class  RangeEqual;
            friend double rangedistance(const Range& c1, const Range& c2);
            friend std::ostream& operator << (std::ostream& output,const Range& c);

            bool operator == (const Range& r) const
            {
                return leftdown_ == r.get_leftdown() && rightup_ == r.get_rightup();
            }

            bool operator != (const Range& r) const
            {
                return ! (*this == r);
            }
        };

        class RangeHash
        {
        public:
            std::size_t operator() (const Range& r) const
            {
                CoordinateHash hash;
                return hash(r.leftdown_)+hash(r.rightup_);
            }
        };

        class RangeEqual
        {
        public:
            bool operator() (const Range& r1,const Range& r2) const
            {
                if(r1.leftdown_ == r2.leftdown_ && r2.rightup_ == r2.rightup_)
                    return  true;
                else
                    return  false;
            }
        };

        double rangedistance(const Range& c1, const Range& c2);
        std::ostream& operator << (std::ostream& output,const Range& c);

    }
}



#endif //NFD_MASTER_RANGE_RANGE_HPP
