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

///@todo make it an inner class of STree and allow for typed weight
template < typename T >
class PTree {
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
    PTree() : weight_(std::numeric_limits< int >::min()), parent_(nullptr),
                data_(T()) {}
    PTree(const T& d, int w) : data_(d), weight_(w), parent_(nullptr) {}
    PTree* Insert(const T& d, int weight, PTree* caller = nullptr, int woff = 0) {
//        std::cout << "===========================\n";
//        if(Root()) Root()->Apply([](T t) { std::cout << t << ' ' << std::endl;});
        weight += woff;
        if(ScopeBegin(d)) woff += weight;
        else if(ScopeEnd(d)) woff -= weight;
        PTree* ret = nullptr;
        if(weight >= weight_) {
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
                (*i)->children_.push_back(p);
                p->parent_ = *i;
                ret = *i;
            }
        } else {
            if(parent_) {
                ret = parent_->Insert(d, weight, caller, woff);
            } else {
                parent_ = new PTree(d, weight);
                parent_->children_.push_back(this);
                ret = parent_;
            }
        }
        
       
        return ret;
    }
    template < typename F >
    F Apply(F f) {
        f(data_);
        for(auto i: children_) i->Apply(f);
        return f;
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
    int weight_; //children's weight is >= current weight
    PTree* parent_;
    std::vector< PTree* > children_;
    int scopeWeightOffset_ = 0;
};

template < typename T > class STree {
public:
//    STree(const STree& t) : tree_(new PTree< T >(*t.tree_)) {}
//    STree(STree&& t) : tree_(t.tree_) { t.tree_ = nullptr; }
//STree() = default;
    STree& Add(const T& data, int weight) {
        if(!tree_) {
            tree_ = new PTree< T >(data, weight);
        } else {
            tree_ = tree_->Insert(data, weight);
        }
        return *this;
    }
    template < typename F >
    F Apply(F f) {
        if(!tree_) return f;
        return tree_->Root()->Apply(f);
    }
    ~STree() { delete tree_; }
private:
    PTree< T >* tree_ = nullptr;
};

//Eval(PTree* r, const FMap& fm) {
//    return fm[r->data_.type](r);
//}


