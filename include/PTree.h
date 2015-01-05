////////////////////////////////////////////////////////////////////////////////
//Copyright (c) 2010-2015, Ugo Varetto
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the copyright holder nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <cassert>

template < typename R, typename N >
R GetData(const N& n);

template < typename T, typename N >
T GetType(const N& n);

///@todo make it an inner class of STree and allow for typed weight
template < typename T, typename WeightT = int, typename OffT = WeightT >
class PTree {
public:
    using Weight = WeightT;
    using Offset = OffT;
public:
    PTree(const PTree& t) : data_(t.data_), weight_(t.weight_),
         parent_(nullptr) {
        for(auto i: t.children_) {
            children_.push_back(new PTree(*i));
            children_.back()->parent_ = this;
        }
    }
    PTree(PTree&& t) : data_(t.data_), weight_(t.weight_), parent_(nullptr) {
        children_ = std::move(t.children_);
        for(auto i: children_) {
            i->parent_ = this;
        }
    }
    PTree() = delete;
    PTree(const T& d, Weight w) : data_(d), weight_(w), parent_(nullptr) {}
    PTree* Insert(const T& d, Weight weight, Offset& offset,
                  PTree* caller = nullptr) {
        PTree* ret = nullptr;
        const Offset off = Offset(weight);
        weight = weight + offset;

        if(weight > weight_) {
            if(std::find(children_.begin(), children_.end(), caller)
               == children_.end()) {
                children_.push_back(new PTree(d, weight));
                children_.back()->parent_ = this;
                ret = children_.back();
            } else {
                typename std::vector< PTree* >::iterator i =
                    std::find(children_.begin(), children_.end(), caller);
                PTree< T >* p = *i;
                *i = new PTree(d, weight);
                (*i)->parent_ = this;
                (*i)->children_.push_back(p);
                p->parent_ = *i;
                ret = *i;
            }
        } else {
            if(parent_) {
                //offset_ is added by default at each invocation of
                //Insert: in case insert is invoked from within insert
                //do remove offset_ offset
                ret = parent_->Insert(d, weight - offset, offset, this);
            } else {
                parent_ = new PTree(d, weight);
                parent_->children_.push_back(this);
                ret = parent_;
            }
        }
        if(ScopeBegin(d)) offset += off;
        else if(ScopeEnd(d)) offset -= off;
        return ret;
    }
    template < typename F >
    F Apply(F f) {
        f(data_);
        for(auto i: children_) i->Apply(f);
        return f;
    }
    template < typename U, typename D, typename M >
    D Eval(const M& fm)  {
        assert(fm.find(GetType< U, T >(data_)) != fm.end());
        auto f = fm.find(GetType< U, T >(data_));
        D r = f(children_.begin()->Eval(fm),
                GetData< D, T >(data_));
        typename std::vector< PTree< T >* >::iterator i = children_.begin();
        ++i;
        for(i; i != children_.end(); ++i) r = f(r, i->Eval(fm));
        return r;
    }
    const PTree< T >* Root() const {
        if(parent_ == nullptr) return this;
        return parent_->Root();
    }
    PTree< T >* Root() {
        if(parent_ == nullptr) return this;
        return parent_->Root();
    }
    const T& Data() const { return data_; }
    const std::vector< PTree* >& Children() const { return children_; }
    ~PTree() {
        for(auto i: children_) delete i;
    }
private:
    T data_;
    Weight weight_; //children's weight is > current weight
    PTree* parent_;
    std::vector< PTree* > children_;
};


template < typename T, typename WM = std::map< int, int > > class STree {
public:
    STree(const STree& t) : tree_(new PTree< T >(*t.tree_)),
                            weights_(t.weights_),
                            offset_(t.offset_) {}
    STree(STree&& t) : tree_(t.tree_),
                       weights_(std::move(t.weights_)),
                       offset_(t.offset_) { t.tree_ = nullptr; }
    STree(const WM& wm) : weights_(wm) {}
    STree() = default;
    STree& Add(const T& data, int weight) {
        if(!tree_) {
            tree_ = new PTree< T >(data, weight);
        } else {
            tree_ = tree_->Insert(data, weight, offset_);
        }
        return *this;
    }
    STree& Add(const T& data) {
        assert(weights_.find(data) != weights_.end());
        return Add(data, weights_[data]);
    }
    template < typename F >
    F Apply(F f) {
        if(!tree_) return f;
        return tree_->Root()->Apply(f);
    }
    void SetWeights(const WM& wm) { weights_ = wm; }
    ~STree() { delete tree_; }
private:
    WM weights_;
    PTree< T >* tree_ = nullptr;
    int offset_ = 0;
};




//Eval(PTree* r, const FMap& fm) {
//    return fm[r->data_.type](r);
//}


