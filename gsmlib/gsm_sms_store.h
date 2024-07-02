// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sms_store.h
// *
// * Purpose: SMS functions, SMS store
// *          (ETSI GSM 07.05)
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 20.5.1999
// *************************************************************************

#ifndef GSM_SMS_STORE_H
#define GSM_SMS_STORE_H

#include <string>
#include <iterator>
#include <gsmlib/gsm_at.h>
#include <gsmlib/gsm_util.h>
#include <gsmlib/gsm_sms.h>
#include <gsmlib/gsm_cb.h>

namespace gsmlib
{
  // forward declarations
  class SMSStore;
  class MeTa;

  // a single entry in the SMS store

  class SMSStoreEntry : public RefBase
  {
  public:
    // status in ME memory
    enum SMSMemoryStatus {ReceivedUnread = 0, ReceivedRead = 1,
                          StoredUnsent = 2, StoredSent = 3,
                          All = 4, Unknown = 5};

  private:
    SMSMessageRef _message;
    SMSMemoryStatus _status;
    bool _cached;
    SMSStore *_mySMSStore;
    int _index;

  public:
    // this constructor is only used by SMSStore
    SMSStoreEntry();

    // create new entry given a SMS message
    SMSStoreEntry(SMSMessageRef message) :
      _message(message), _status(Unknown), _cached(true), _mySMSStore(NULL),
      _index(0) {}

    // create new entry given a SMS message and an index
    // only to be used for file-based stores (see gsm_sorted_sms_store)
    SMSStoreEntry(SMSMessageRef message, int index) :
      _message(message), _status(Unknown), _cached(true), _mySMSStore(NULL),
      _index(index) {}
   
    // clear cached flag
    void clearCached() { _cached = false; }

    // return SMS message stored in the entry
    SMSMessageRef message() const ;

    // return CB message stored in the entry
    CBMessageRef cbMessage() const ;

    // return message status in store
    SMSMemoryStatus status() const ;

    // return true if empty, ie. no SMS in this entry
    bool empty() const ;

    // send this PDU from store
    // returns message reference and ACK-PDU (if requested)
    // only applicate to SMS-SUBMIT and SMS-COMMAND
    unsigned char send(Ref<SMSMessage> &ackPdu) ;
    
    // same as above, but ACK-PDU is discarded
    unsigned char send() ;

    // return index (guaranteed to be unique,
    // can be used for identification in store)
    int index() const {return _index;}

    // return true if entry is cached (and caching is enabled)
    bool cached() const;

    // return deep copy of this entry
    Ref<SMSStoreEntry> clone();

    // equality operator
    bool operator==(const SMSStoreEntry &e) const;

    // return store reference
    SMSStore *getStore() {return _mySMSStore;}

    // copy constructor and assignment
    SMSStoreEntry(const SMSStoreEntry &e);
    SMSStoreEntry &operator=(const SMSStoreEntry &e);

    friend class SMSStore;
  };

  // iterator for the SMSStore class

#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
  class SMSStoreIterator : public random_access_iterator<SMSStoreEntry,
                           int>
#else
  class SMSStoreIterator : public std::iterator<std::random_access_iterator_tag,
                           SMSStoreEntry, int>
#endif
  {
    int _index;
    SMSStore *_store;

    SMSStoreIterator(int index, SMSStore *store) :
      _index(index), _store(store) {}

  public:
    SMSStoreIterator(SMSStoreEntry *entry) :
      _index(entry->index()), _store(entry->getStore()) {}

    SMSStoreEntry &operator*();
    SMSStoreEntry *operator->();
    SMSStoreIterator &operator+(int i)
      {_index += i; return *this;}
    operator SMSStoreEntry*();
    SMSStoreIterator &operator=(const SMSStoreIterator &i);
    SMSStoreIterator &operator++()
      {++_index; return *this;}
    SMSStoreIterator &operator--()
      {--_index; return *this;}
    SMSStoreIterator &operator++(int i)
      {_index += i; return *this;}
    SMSStoreIterator &operator--(int i)
      {_index -= i; return *this;}
    bool operator<(SMSStoreIterator &i)
      {return _index < i._index;}
    bool operator==(const SMSStoreIterator &i) const
      {return _index == i._index;}
    bool operator!=(const SMSStoreIterator &i) const
      {return _index != i._index;}

    friend class SMSStore;
  };

#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
  class SMSStoreConstIterator : public std::random_access_iterator<SMSStoreEntry,
                                int>
#else
  class SMSStoreConstIterator : public std::iterator<std::random_access_iterator_tag,
                                SMSStoreEntry, int>
#endif
  {
    int _index;
    const SMSStore *_store;

    SMSStoreConstIterator(int index, const SMSStore *store) :
      _index(index), _store(store) {}

  public:
    const SMSStoreEntry &operator*();
    const SMSStoreEntry *operator->();
    SMSStoreConstIterator &operator++()
      {++_index; return *this;}
    SMSStoreConstIterator &operator--()
      {--_index; return *this;}
    SMSStoreConstIterator &operator++(int i)
      {_index += i; return *this;}
    SMSStoreConstIterator &operator--(int i)
      {_index -= i; return *this;}
    bool operator<(SMSStoreConstIterator &i)
      {return _index < i._index;}
    bool operator==(const SMSStoreConstIterator &i) const
      {return _index == i._index;}

    friend class SMSStore;
  };

  // this class corresponds to a SMS store in the ME
  // all functions directly update storage in the ME
  // if the ME is exchanged, the storage may become corrupted because
  // of internal buffering in the SMSStore class

  class SMSStore : public RefBase, public NoCopy
  {
  private:
    std::vector<SMSStoreEntry*> _store; // vector of store entries
    std::string _storeName;          // name of the store, 2-byte like "SM"
    Ref<GsmAt> _at;             // my GsmAt class
    MeTa &_meTa;                // my MeTa class
    bool _useCache;             // true if entries should be cached

    // internal access functions
    // read/write entry from/to ME
    void readEntry(int index, SMSMessageRef &message,
                   SMSStoreEntry::SMSMemoryStatus &status) ;
    void readEntry(int index, CBMessageRef &message) ;
    void writeEntry(int &index, SMSMessageRef message)
      ;
    // erase entry
    void eraseEntry(int index) ;
    // send PDU index from store
    // returns message reference and ACK-PDU (if requested)
    // only applicate to SMS-SUBMIT and SMS-COMMAND
    unsigned char send(int index, Ref<SMSMessage> &ackPdu) ;
    

    // do the actual insertion, return index of new element
    int doInsert(SMSMessageRef message) ;

    // used by class MeTa
    SMSStore(std::string storeName, Ref<GsmAt> at, MeTa &meTa) ;

    // resize store entry vector if necessary
    void resizeStore(int newSize);

  public:
    // iterator defs
    typedef SMSStoreIterator iterator;
    typedef SMSStoreConstIterator const_iterator;
    typedef SMSStoreEntry &reference;
    typedef const SMSStoreEntry &const_reference;

    // set cache mode on or off
    void setCaching(bool useCache) {_useCache = useCache;}

    // return name of this store (2-character string)
    std::string name() const {return _storeName;}

    // SMS store traversal commands
    // these are suitable to use stdc++ lib algorithms and iterators
    // ME have fixed storage space implemented as memory slots
    // that may either be empty or used
    
    // traversal commands
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;
    reference operator[](int n);
    const_reference operator[](int n) const;

    // The size macros return the number of used entries
    // Warning: indices may be _larger_ than size() because of this
    // (perhaps this should be changed, because it is unexpected behavior)

    int size() const ;
    int max_size() const {return _store.size();}
    int capacity() const {return _store.size();}
    bool empty() const  {return size() == 0;}

    // insert iterators insert into the first empty cell regardless of position
    // existing iterators may be invalidated after an insert operation
    // return position
    // insert only writes to available positions
    // warning: insert fails silently if size() == max_size()
    iterator insert(iterator position, const SMSStoreEntry& x)
      ;
    iterator insert(const SMSStoreEntry& x) ;

    // insert n times, same procedure as above
    void insert (iterator pos, int n, const SMSStoreEntry& x)
      ;
    void insert (iterator pos, long n, const SMSStoreEntry& x)
      ;

    // erase operators set used slots to "empty"
    iterator erase(iterator position) ;
    iterator erase(iterator first, iterator last) ;
    void clear() ;

    // destructor
    ~SMSStore();

    friend class SMSStoreEntry;
    friend class MeTa;
  };

  typedef Ref<SMSStore> SMSStoreRef;

};

#endif // GSM_SMS_STORE_H
