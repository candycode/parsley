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
#include <stack>


namespace parsley {

enum class APPLY {BEGIN, END};    
    
template < typename F >
struct Result {
    using Type = typename F::result_type;
};
    
template < typename R, typename... Args >
struct Result< R (*)(Args...) > {
    using Type = R;
};
    
///@todo make it an inner class of STree
template < typename T, typename WeightT = int, typename OffT = WeightT >
class WTree {
public:
    using Weight = WeightT;
    using Offset = OffT;
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
    WTree* Insert(const T& d, Weight weight, WTree* caller = nullptr) {
        WTree* ret = nullptr;
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
            if(parent_) ret = parent_->Insert(d, weight, this);
            else {
                parent_ = new WTree(d, weight);
                parent_->children_.push_back(this);
                ret = parent_;
            }
        }
        return ret;
    }
    //move to node with weight == to
    WTree* Rewind(Weight w) {
        if(w >= weight_ || !parent_) return this;
        else return parent_->Rewind(w);
    }
    template < typename F >
    F Apply(F f) {
        f(data_);
        for(auto i: children_) i->Apply(f);
        return f;
    }
    //scoped apply: functor is applied to current node first then to children,
    //then to current node again at the end
    template < typename F >
    F ScopedApply(F f) {
        auto a(f(data_, APPLY::BEGIN));
        for(auto i: children_) a = i->ScopedApply(a);
        return a(data_, APPLY::END);
    }
    //functional scoped apply: function is returned which traverses tree when
    //invoked
    template < typename F >
    std::function< F () > FunScopedApply(F f) {
        auto g = [f, this]() {
            auto a(std::move(f(data_, APPLY::BEGIN)));
            for(auto i: children_) a = i->ScopedApply(a);
            return a(data_, APPLY::END);
        };
        return g;
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
    STree& Add(const T& data, WT weight, bool scopeAdd) {
        weight += offset_;
        if(ScopeBegin(data)) offset_ += weight;
        else if(ScopeEnd(data)) offset_ -= weight;
        if((ScopeBegin(data) || ScopeEnd(data)) && !scopeAdd) return *this;
        if(!root_) {
            tree_ = new WTree< T >(data, weight);
        } else {
            tree_ = tree_->Insert(data, weight);
        }
        root_.release();
        root_ = TPtr(tree_->Root());
        return *this;
    }
    STree& Add(const T& data, bool scopeAdd = false) {
        assert(weights_.find(GetType(data)) != weights_.end());
        return Add(data, weights_.find(GetType(data))->second, scopeAdd);
    }
    ///Rewind: no offset is taken into account, use Offset() to add
    ///offset before calling this method as needed
    STree& Rewind(WT weight) {
        assert(root_);
        assert(tree_);
        if(marks_.size() > 1) weight += offset_;
        tree_ = tree_->Rewind(weight);
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
    template < typename F >
    F ScopedApply(F&& f) {
        return root_->ScopedApply(std::forward< F >(f));
    }
    void OffsetInc(typename WM::value_type::first_type t) {
        offset_ += weights_[t];
    }
    void OffsetDec(typename WM::value_type::first_type t) {
        offset_ -= weights_[t];
    }
    void SaveOffset() { marks_.push(offset_); }
    void ResetOffset() { offset_ = marks_.top(); marks_.pop(); }
    void SetWeights(const WM& wm) { weights_ = wm; }
    void Reset() { root_.reset(nullptr); offset_ = Offset(0); tree_ = nullptr; }
    OFFT GetOffset() const { return offset_; }
private:
    using Tree = WTree< T, WT, OFFT>;
    using TPtr = std::unique_ptr< Tree > ;
    using Offset = OFFT;
    WM weights_;
    TPtr root_;
    Tree* tree_ = nullptr;
    Offset offset_ = Offset(0);
    std::stack< Offset > marks_;
};
}

