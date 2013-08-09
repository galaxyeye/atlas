//          Copyright Stefan Strasser 2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_EXCEPTION_HEADER_HPP
#define BOOST_TRANSACT_EXCEPTION_HEADER_HPP

#include <exception>
#include <boost/assert.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/transact/detail/algorithm.hpp>


namespace boost{
namespace transact{

///\brief Exception base class.
struct exception : std::exception{};

///\brief Indicates that a persistent resource recovery failed.
struct recovery_failure : transact::exception{};

///\brief Indicates that an internal operation reading from/writing to files failed.
struct io_failure : transact::exception{};

///\brief Indicates that this operation required a transaction but there was no transaction bound to this thread, or that the operation
///required an active transaction but the transaction bound to this thread was inactive.
struct no_transaction : transact::exception{};

///\brief Indicates an error with regard to connecting a resource to a transaction manager
struct resource_error : transact::exception{};

///\brief Indicates that no transaction manager was constructed.
struct no_transaction_manager : transact::exception{};

///\brief Indicates that this operation is not supported by this implementation
struct unsupported_operation : transact::exception{};

struct isolation_exception;
template<class ResMgr>
struct resource_isolation_exception;

///\brief Indicates that the operation conflicted with another transaction.
///
///\c isolation_exception is an abstract base class. The derived class
///\c resource_isolation_exception can be used to throw this exception.
struct isolation_exception : transact::exception{
    ///Rethrows the exception if the current transaction is a nested transaction but the isolation exception was caused by a parent transaction of it,
    ///or if the isolation_exception was caused independently of a transaction.
    ///\pre TxMgr::current_transaction() must be a rolled back transaction
    template<class TxMgr>
    void unwind() const{ //pseudo-virtual
        typedef typename TxMgr::resource_types res_types;
        BOOST_VERIFY(( ! detail::for_each_if<res_types>(unwinder<TxMgr>(*this)) ));
    }
    virtual ~isolation_exception() throw(){}
protected:
    isolation_exception(){}
private:
    template<class TxMgr>
    struct unwinder{
        explicit unwinder(isolation_exception const &e) : e(e){}
        template<class Pair>
        bool operator()() const{
            typedef resource_isolation_exception<typename Pair::second> der_type;
            if(der_type const *der=dynamic_cast<der_type const *>(&this->e)){
                der->template unwind<TxMgr>();
                return false;
            }else return true;
        }
    private:
        isolation_exception const &e;
    };
};


///\brief Indicates that the operation conflicted with another transaction.
///
///The base class \c isolation_exception should be used to catch this exception,
///in order to catch isolation exceptions of all resource managers.
template<class ResMgr>
struct resource_isolation_exception : isolation_exception{
    ///\brief Constructs a resource_isolation_exception
    resource_isolation_exception()
        : res(0),tx(0){}

    ///\brief Constructs a resource_isolation_exception
    ///\param res The resource manager that is throwing the exception
    ///\param retry The transaction that caused the isolation_exception and ought to be repeated.
    ///Must be a transaction on the nested transaction stack.
    explicit resource_isolation_exception(ResMgr const &res,typename ResMgr::transaction const &retry)
        : res(&res),tx(&retry){}

    ///\brief Equivalent to <tt>isolation_exception::unwind<TxMgr>()</tt>
    template<class TxMgr>
    void unwind() const{ //pseudo-virtual
        if(this->res){
            typedef typename TxMgr::resource_types res_types;
            unwinder<TxMgr> unw(*this->res,*this->tx);
            BOOST_VERIFY(( ! detail::for_each_if<res_types>(unw) ));
        }else throw;
    }
    virtual ~resource_isolation_exception() throw(){}
private:
    template<class TxMgr>
    struct unwinder{
        unwinder(ResMgr const &res,typename ResMgr::transaction const &rtx) : res(res),rtx(rtx){}
        template<class Pair>
        bool operator()() const{
            return this->operator()<typename Pair::first>(typename is_same<typename Pair::second,ResMgr>::type());
        }
    private:
        template<class Tag>
        bool operator()(mpl::true_) const{
            typedef typename TxMgr::template resource_iterator<Tag>::type iterator;
            std::pair<iterator,iterator> range=TxMgr::template resources<Tag>();
            for(iterator it=range.first;it != range.second;++it){
                if(it->second == &this->res){
                    typename TxMgr::transaction *tx=TxMgr::current_transaction();
                    BOOST_ASSERT(tx);
                    typename ResMgr::transaction &foundrtx=TxMgr::resource_transaction(*tx,it->first);
                    if(&foundrtx != &this->rtx){
                        //the transaction that ought to be retried must be a parent of tx:
                        BOOST_ASSERT(TxMgr::parent_transaction(*tx));
                        throw;
                    }else{
                        //the following only works if the TM either uses flat nested txs for all RMs or for none:
                        typename TxMgr::transaction *parent=TxMgr::parent_transaction(*tx);
                        if(parent && &foundrtx == &TxMgr::resource_transaction(*parent,it->first)){
                            //the resource transaction in tx is the same resource transaction as in the parent of tx.
                            //this means the TM is using flat nested transactions. the outermost transaction using rtx must be repeated, not this one:
                            throw;
                        }
                    }
                    return false;
                }
            }
            return true;
        }
        template<class Pair>
        bool operator()(mpl::false_) const{
            // Pair does correspond to another type of resource manager, continue iterating:
            return true;
        }

        ResMgr const &res;
        typename ResMgr::transaction const &rtx;
    };
    //always: bool(res) == bool(tx)
    ResMgr const *res;
    typename ResMgr::transaction const *tx;
};

}
}



#endif
