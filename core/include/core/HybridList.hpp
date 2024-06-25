/*
 * Copyright (C) 2024 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *    http://www.apache.org/licenses/LICENSE-2.0 

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef HYBRID_LIST_LIBRARY_H
#define HYBRID_LIST_LIBRARY_H

#include <list>
#include <array>

/*!
 * A container that can dynamically grow in size and never invalidates iterators.
 * It grows by block, letting the previous blocks in place.
 * It can only store copy assignable types.
 *
 * @tparam value_t      Copy assignable type stored in the container
 * @tparam array_size   Size of the blocks
 */
template <class value_t, size_t array_size = 1024>
class HybridList
{
    static_assert(std::is_copy_assignable<value_t>::value,
                  "value_t must be copy_assignable");
    static_assert(array_size > 0,
                  "array_size must be > 0");

    using array_type = std::array<value_t, array_size>;
    using container_type = std::list<array_type>;

    container_type mContainer;
    size_t mCurrentIdx;

    //! Forward iterator
    template <typename T>
    class Iterator
    {
        typename container_type::iterator mIt;
        typename array_type::iterator mSubIt;

    public:
        using iterator_category =  std::forward_iterator_tag ;

        explicit Iterator(HybridList& hl, bool end = false) :
                mIt(end ? --hl.mContainer.end() : hl.mContainer.begin()),
                mSubIt(end ? mIt->begin() + hl.mCurrentIdx :
                       hl.mContainer.begin()->begin())
        {}

        Iterator operator++() {
            auto i = *this;
            mSubIt++;
            if(mSubIt == mIt->end()){
                mIt++;
                mSubIt = mIt->begin();
            }
            return i;
        }

        Iterator operator++(int) {
            mSubIt++;
            if(mSubIt == mIt->end()){
                mIt++;
                mSubIt = mIt->begin();
            }
            return *this;
        }

        T& operator*() { return *mSubIt; }
        T* operator->() { return mSubIt; }
        bool operator==(const Iterator& rhs) { return mSubIt == rhs.mSubIt; }
        bool operator!=(const Iterator& rhs) { return mSubIt != rhs.mSubIt; }
    };
    template<typename> friend class Iterator;

public:
    using value_type = value_t;
    using iterator = HybridList::Iterator<value_type>;
    using const_iterator = HybridList::Iterator<const value_type>;

    HybridList(): mContainer(1), mCurrentIdx(0){}

    template<class T>
    value_t& emplace_back(T &&val){
        if(mCurrentIdx >= array_size){
            mContainer.emplace_back(array_type());
            mCurrentIdx = 0;
        }

        auto& array = *(mContainer.rbegin());
        return array[mCurrentIdx++] = std::forward<T>(val);
    }

    iterator begin(){
        return iterator(*this);
    }
    iterator end(){
        return iterator(*this, true);
    }
    const_iterator cbegin(){
        return const_iterator(*this);
    }
    const_iterator cend(){
        return const_iterator(*this, true);
    }
};

#endif