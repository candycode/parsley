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
#include <functional>

namespace parsley {

///@todo make it an inner class of STree and allow for typed weight
template < typename T, typename WeightT = int, typename OffT = WeightT >
class WTree {
public:
    using Weight = WeightT;
    using Offset = OffT;
    using UUID = void*; //unique, per-process id
public:
    WTree(const WTree& t) : data_(t.data_), weight_(t.weight_),
         parent_(nullptr) {
        for(auto i: t.children_) {
            children_.push_back(new WTree(*i));
            children_.back()->parent_ = this;
        }
    }
    WTree(WTree&& t) : data_(t.data_), weight_(t.weight_), parent_(nullptr) {
        children_ = std::move(t.children_);
        for(auto i: children_) {
            i->parent_ = this;
        }
    }
    WTree() = delete;
    WTree(const T& d, Weight w) : data_(d), weight_(w), parent_(nullptr) {}
    WTree* Insert(const T& d, Weight weight, Offset& offset,
                  WTree* caller = nullptr) {
        WTree* ret = nullptr;
        const Offset off = Offset(weight);
        weight = weight + offset;

        if(weight > weight_) {
            if(std::find(children_.begin(), children_.end(), caller)
               == children_.end()) {
                children_.push_back(new WTree(d, weight));
                children_.back()->parent_ = this;
                ret = children_.back();
            } else {
                typename std::vector< WTree* >::iterator i =
                    std::find(children_.begin(), children_.end(), caller);
                WTree< T >* p = *i;
                *i = new WTree(d, weight);
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
                parent_ = new WTree(d, weight);
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
    template < typename DataT,
               typename FunctionMapT,
               typename InitDataF,
               typename GetDataF,
               typename GetTypeF >
    DataT Eval(const FunctionMapT& fm,
               const InitDataF& init,
               const GetDataF& getData,
               const GetTypeF& getType)  {
        using Type = typename FunctionMapT::value_type::first_type;
        assert(fm.find(getData(data_)) != fm.end());
        auto f = fm.find(getType(data_));
        DataT r = init(getType(data_));
        if(children_.size() > 0) {
            r = f(children_.begin()->Eval(fm),
                  getData(data_));
            typename std::vector< WTree< T >* >::iterator i = children_.begin();
            ++i;
            for(i; i != children_.end(); ++i) r = f(r, i->Eval(fm));
        } else {
            r = f(r, getData(data_));
        }
        return r;
    }
//    //Deferred execution: stores evaluation path and data in function
//    //closure, valid until all tree nodes are not deleted
//    template < typename DataT, typename FunctionMapT >
//    std::function< DataT() > EvalFun(const FunctionMapT& fm)  {
//        using Type = typename FunctionMapT::value_type::first_type;
//        assert(fm.find(GetType< Type, T >(data_)) != fm.end());
//        auto f = fm.find(GetType< Type, T >(data_));
//        DataT r = InitData< DataT, Type >();
//        std::function< DataT() > F;
//        if(children_.size() > 0) {
//            typename std::vector< WTree< T >* >::iterator i = children_.begin();
//            F = [i, f, r, &fm]() {
//                return f(i->EvalFun(fm)(), r);
//            };
//            ++i;
//            for(i; i != children_.end(); ++i) {
//                F = [f, F, i, &fm] {
//                    return f(i->EvalFun(fm)(), F());
//                };
//            }
//        } else {
//            r = f(r, GetData< DataT, T >(data_));
//        }
//        return F;
//    }
    const WTree< T >* Root() const {
        if(parent_ == nullptr) return this;
        return parent_->Root();
    }
    WTree< T >* Root() {
        if(parent_ == nullptr) return this;
        return parent_->Root();
    }
    const T& Data() const { return data_; }
    const std::vector< WTree* >& Children() const { return children_; }
    ~WTree() {
        for(auto i: children_) delete i;
    }
    //copies tree, it also returns the new address assigned to the copy
    //of the node passed as a parameter; this is required because in case
    //a pointer points to a tree node we want to know the address of the
    //newly created node in the new tree
    WTree* DeepCopy(WTree* in, WTree*& out) {
        WTree* p = new WTree(data_, weight_);
        for(auto i: children_) {
            p->children_.push_back(i->DeepCopy(in, out));
        }
        if(this == in) out = p;
        return p;
    }
private:
    T data_;
    Weight weight_; //children's weight is > current weight
    WTree* parent_;
    std::vector< WTree* > children_;
};


template < typename T,
           typename WM = std::map< int, int >,
           typename WT = typename WM::value_type::second_type,
           typename OFFT = WT >
class STree {
public:
    //copy constructor, copies tree and also makes the tree_ pointer point
    //to the correct node in the newly created tree
    STree(const STree& t) : weights_(t.weights_), tree_(nullptr),
                            offset_(t.offset_) {
        if(t.tree_) {
            root_ = TPtr(t.tree_->Root()->DeepCopy(t.tree_, tree_));
        }
    }
    STree(STree&& t) : tree_(t.tree_),
                       root_(std::move(t.root_)),
                       weights_(std::move(t.weights_)),
                       offset_(t.offset_) {}
    STree& operator=(STree s) {
        weights_ = std::move(s.weights);
        root_ = std::move(s.root_);
        offset_ = s.offset_;
        tree_ = s.tree_;
        return *this;
    }
    STree(const WM& wm) : weights_(wm) {}
    STree() = default;
    STree& Add(const T& data, int weight) {
        if(!tree_) {
            tree_ = new WTree< T >(data, weight);
        } else {
            tree_ = tree_->Insert(data, weight, offset_);
        }
        root_.release();
        root_ = TPtr(tree_->Root());
        return *this;
    }
    template < typename D, typename U >
    friend D Get(const U& );
    //ADD ACCESSORS
    STree& Add(const T& data) {
        assert(weights_.find(data) != weights_.end());
        return Add(data, weights_[data]);
    }
    template < typename DataT,
    typename FunctionMapT,
    typename InitDataF,
    typename GetDataF,
    typename GetTypeF >
    DataT Eval(const FunctionMapT& fm,
               const InitDataF& init,
               const GetDataF& getData,
               const GetTypeF& getType) {
        assert(root_);
        return root_->template Eval< DataT >(fm, init, getData, getType);
    }
    template < typename F >
    F Apply(F f) {
        assert(root_);
        return root_->Apply(f);
    }
    void SetWeights(const WM& wm) { weights_ = wm; }
private:
    using Tree = WTree< T, WT, OFFT>;
    using TPtr = std::unique_ptr< Tree > ;
    using Offset = OFFT;
    WM weights_;
    TPtr root_;
    Tree* tree_ = nullptr;
    Offset offset_ = 0;
};
}

