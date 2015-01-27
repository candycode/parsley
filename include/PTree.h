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
#include <type_traits>


namespace parsley {

template < typename F >
struct Result {
    using Type = typename F::result_type;
};
    
template < typename R, typename... Args >
struct Result< R (*)(Args...) > {
    using Type = R;
};
    
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
    //move to node with weight == to
    WTree* Rewind(Weight w, Offset& offset) {
        w = w + offset;
        if(w >= weight_ || !parent_) return this;
        else return parent_->Rewind(w - offset, offset);
    }
    template < typename F >
    F Apply(F f) {
        f(data_);
        for(auto i: children_) i->Apply(f);
        return f;
    }
    //scoped apply: functor is applied to current node first then to children,
    //then to current node again at the end
    enum class APPLY {BEGIN, END};
    template < typename F >
    F ScopedApply(F&& f) {
        F a = f(data_, APPLY::BEGIN);
        for(auto i: children_) a = i->ScopedApply(a);
        return a(data_, APPLY::END);
    }
    //Evaluation order:
    //if node has children:
    //  initialize result <- value of first child
    //  at each iteration result <- F(result, child->Eval)
    //if node has NO children:
    //  initialize result with Init specialization: result <- Init(type)
    //  return F(node data, result)
    template < typename FunctionMapT >
    typename Result<
        typename FunctionMapT::value_type::second_type >::Type
    Eval(const FunctionMapT& fm)  {
        using Type = typename FunctionMapT::value_type::first_type;
        assert(fm.find(GetType(data_)) != fm.end());
        auto& f = fm.find(GetType(data_))->second;
        using DataT = typename Result<
            typename FunctionMapT::value_type::second_type >::Type;
        DataT r = Init(GetType(data_));
        if(children_.size() > 0) {
            r = (*children_.begin())->Eval(fm);
            typename std::vector< WTree< T >* >::iterator i
                = children_.begin();
            ++i;
            for(; i != children_.end(); ++i) r = f(r, (*i)->Eval(fm));
        } else r = f(GetData(data_), r);
        return r;
    }
    //Iterative eval function: at each node the result of children evaluation
    //is stored into array together with data value at current node; function
    //is then called with array
    template < typename FunctionMapT >
    auto EvalArgs(const FunctionMapT& fm) -> decltype(GetData(T())) {
        using Type = typename FunctionMapT::value_type::first_type;
        assert(fm.find(GetType(data_)) != fm.end());
        auto f = fm.find(GetType(data_))->second;
        using DataT = typename Result<
            typename FunctionMapT::value_type::second_type >::Type;
        std::vector< DataT > args;
        args.reserve(1 + children_.size());
        args.push_back(GetData(data_));
        typename std::vector< WTree< T >* >::iterator i = children_.begin();
        for(; i != children_.end(); ++i) args.push_back(i->EvalArgs(fm));
        return Get(GetType(data_), fm)(args);
    }
#if 0
    //Deferred execution: stores evaluation path and data in function
    //closure, valid until all tree nodes are not deleted
    template < typename FunctionMapT >
    std::function< typename Result<
        typename FunctionMapT::value_type::second_type >::Type () >
    EvalFun(const FunctionMapT& fm)  {
        using DataT = typename Result<
            typename FunctionMapT::value_type::second_type >::Type;
        using Type = typename FunctionMapT::value_type::first_type;
        assert(fm.find(GetType(data_)) != fm.end());
        auto f = fm.find(GetType(data_))->second;
        DataT r = Init(GetType(data_));
        std::function< DataT() > F;
        if(children_.size() > 0) {
            typename std::vector< WTree< T >* >::iterator i = children_.begin();
            F = [i, f, r, &fm]() {
                return f(i->EvalFun(fm)(), r);
            };
            ++i;
            for(i; i != children_.end(); ++i) {
                F = [f, F, i, &fm] {
                    return f(i->EvalFun(fm)(), F());
                };
            }
        } else {
            const DataT c = GetData(data_);
            F = [f, r, &fm, c]() {
                return f(r, c);
            };
        }
        return
    }
#endif
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
    STree& Add(const T& data, WT weight) {
        if(!root_) {
            tree_ = new WTree< T >(data, weight);
            if(ScopeBegin(data)) offset_ = OFFT(weight);
        } else {
            tree_ = tree_->Insert(data, weight, offset_);
        }
        root_.release();
        root_ = TPtr(tree_->Root());
        return *this;
    }
    STree& Add(const T& data) {
        assert(weights_.find(GetType(data)) != weights_.end());
        return Add(data, weights_.find(GetType(data))->second);
    }
    STree& Rewind(WT weight) {
        assert(root_);
        assert(tree_);
        tree_ = tree_->Rewind(weight, offset_);
        return *this;
    }
    template < typename FunctionMapT >
    typename Result<
        typename FunctionMapT::value_type::second_type >::Type
    Eval(const FunctionMapT& fm ) {
        assert(root_);
        return root_->Eval(fm);
    }
    template < typename F >
    F Apply(F f) {
        assert(root_);
        return root_->Apply(f);
    }
    void SetWeights(const WM& wm) { weights_ = wm; }
    void Reset() { root_.reset(nullptr); offset_ = Offset(0); tree_ = nullptr; }
private:
    using Tree = WTree< T, WT, OFFT>;
    using TPtr = std::unique_ptr< Tree > ;
    using Offset = OFFT;
    WM weights_;
    TPtr root_;
    Tree* tree_ = nullptr;
    Offset offset_ = Offset(0);
};
}

