/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

#ifndef _ENVELOPE_H
#define _ENVELOPE_H

#ifndef CINTBITS
#define CINTBITS (sizeof(int)*8)
#endif

#ifndef CMK_OPTIMIZE
#define _SET_USED(env, x) (env)->setUsed((x))
#define _CHECK_USED(env) do { if(env->isUsed()) \
                           CmiAbort("Message being re-sent. Aborting...\n"); \
                         } while(0)
#else
#define _SET_USED(env, x) do{}while(0)
#define _CHECK_USED(env) do{}while(0)
#endif

#define SVM1     (sizeof(double)-1)
#define _ALIGN(x) (((x)+SVM1)&(~(SVM1)))
#define _AA(x)     _ALIGN(x)
#define _DD(x)     (_AA(x)-(x))
#define PW(x)    ((x+CINTBITS-1)/CINTBITS)

#define NewChareMsg    1
#define NewVChareMsg   2
#define BocInitMsg     3
#define ForChareMsg    4
#define ForBocMsg      5
#define ForVidMsg      6
#define FillVidMsg     7
#define RODataMsg      8
#define ROMsgMsg       9
#define ExitMsg        10
#define ReqStatMsg     11
#define StatMsg        12
#define NodeBocInitMsg 13
#define ForNodeBocMsg  14
#define ArrayEltInitMsg 15
#define ForArrayEltMsg  16

typedef unsigned int   UInt;
typedef unsigned short UShort;
typedef unsigned char  UChar;

#include <charm.h> // for CkGroupID

#if CMK_BLUEGENE_CHARM
#define CmiReservedHeaderSize   CmiBlueGeneMsgHeaderSizeBytes
#else
#define CmiReservedHeaderSize   CmiExtHeaderSizeBytes
#endif

class envelope {
  private:
    char   core[CmiReservedHeaderSize];
 //This union allows the different kinds of messages to have different
 // fields/types in an alignment-safe way without wasting any storage.
public:
    union u_type {
      struct s_chare { //NewChareMsg, NewVChareMsg, ForChareMsg, ForVidMsg, FillVidMsg
      	void *ptr;
      	UInt forAnyPe; //Used by new-only
      } chare;
      struct s_group {
	CkGroupID g; //GroupID
	CkNodeGroupID rednMgr; //Reduction manager for this group (constructor only!)
	int epoch; //"epoch" this group was created during (0--mainchare, 1--later)
	UShort arrayEp; // Used only for array broadcasts
      } group;
      struct s_array{ //For arrays only
	CkArrayIndexStruct index;//Array element index
	int listenerData[CK_ARRAYLISTENER_MAXLEN]; //For creation
	CkGroupID arr; //Array manager GID
	UChar hopCount;//number of times message has been routed
    	UChar ifNotThere; //what to do if array element is missing
      } array;
      struct s_roData { //RODataMsg
      	UInt count;
      } roData;
      struct s_roMsg { //ROMsgMsg
      	UInt roIdx;
      } roMsg;
    };
    struct s_attribs { //Packed bitwise struct
    	UChar msgIdx; //Usertype of message (determines pack routine)
	UChar mtype;
    	UChar queueing:4; //Queueing strategy (FIFO, LIFO, PFIFO, ...)
    	UChar isPacked:1;
    	UChar isUsed:1;
	UChar isImmediate:1;   //Used by immediate msgs
    };
private:
    u_type type; //Depends on message type (attribs.mtype)
    UShort ref; //Used by futures
    s_attribs attribs;
    UChar align[_DD(CmiReservedHeaderSize+sizeof(u_type)+sizeof(UShort)+sizeof(s_attribs))];
    
    //This struct should now be sizeof(void*) aligned.
    UShort priobits;
    UShort epIdx;  //Entry point to call
    UInt   pe;    // source processor
    UInt   event; // used by projections
    UInt   totalsize; //Byte count from envelope start to end of priobits
    
  public:
    void pup(PUP::er &p);
    UInt   getEvent(void) const { return event; }
    void   setEvent(const UInt e) { event = e; }
    UInt   getRef(void) const { return ref; }
    void   setRef(const UShort r) { ref = r; }
    UChar  getQueueing(void) const { return attribs.queueing; }
    void   setQueueing(const UChar q) { attribs.queueing=q; }
    UChar  getMsgtype(void) const { return attribs.mtype; }
    void   setMsgtype(const UChar m) { attribs.mtype = m; }
#ifndef CMK_OPTIMIZE
    UChar  isUsed(void) { return attribs.isUsed; }
    void   setUsed(const UChar u) { attribs.isUsed=u; }
#else /* CMK_OPTIMIZE */
    inline void setUsed(const UChar u) {}
#endif
    UChar  isImmediate(void) { return attribs.isImmediate; }
    void   setImmediate(const UChar u) { attribs.isImmediate=u; }
    UChar  getMsgIdx(void) const { return attribs.msgIdx; }
    void   setMsgIdx(const UChar idx) { attribs.msgIdx = idx; }
    UInt   getTotalsize(void) const { return totalsize; }
    void   setTotalsize(const UInt s) { totalsize = s; }
    UChar  isPacked(void) const { return attribs.isPacked; }
    void   setPacked(const UChar p) { attribs.isPacked = p; }
    UShort getPriobits(void) const { return priobits; }
    void   setPriobits(const UShort p) { priobits = p; }
    UShort getPrioWords(void) const { return (priobits+CINTBITS-1)/CINTBITS; }
    UShort getPrioBytes(void) const { return getPrioWords()*sizeof(int); }
    void*  getPrioPtr(void) const { 
      return (void *)((char *)this + totalsize - getPrioBytes());
    }
    static envelope *alloc(const UChar type, const UInt size=0, const UShort prio=0)
    {
      CkAssert(type>=NewChareMsg && type<=ForArrayEltMsg);
      register UInt tsize = sizeof(envelope)+_ALIGN(size)+sizeof(int)*PW(prio);
      register envelope *env = (envelope *)CmiAlloc(tsize);
      env->setMsgtype(type);
      env->totalsize = tsize;
      env->priobits = prio;
      env->setPacked(0);
      env->setImmediate(0);
      _SET_USED(env, 0);
      //for record-replay
      env->setEvent(0);
      return env;
    }
    UShort getEpIdx(void) const { return epIdx; }
    void   setEpIdx(const UShort idx) { epIdx = idx; }
    UInt   getSrcPe(void) const { return pe; }
    void   setSrcPe(const UInt s) { pe = s; }

// Readonly-specific fields
    UInt   getCount(void) const { 
      CkAssert(getMsgtype()==RODataMsg); return type.roData.count; 
    }
    void   setCount(const UInt c) { 
      CkAssert(getMsgtype()==RODataMsg); type.roData.count = c; 
    }
    UInt   getRoIdx(void) const { 
      CkAssert(getMsgtype()==ROMsgMsg); return type.roMsg.roIdx; 
    }
    void   setRoIdx(const UInt r) { 
      CkAssert(getMsgtype()==ROMsgMsg); type.roMsg.roIdx = r; 
    }
    
 // Chare-specific fields
    UInt isForAnyPE(void) { 
      CkAssert(getMsgtype()==NewChareMsg || getMsgtype()==NewVChareMsg); 
      return type.chare.forAnyPe; 
    }
    void setForAnyPE(UInt f) { 
      CkAssert(getMsgtype()==NewChareMsg || getMsgtype()==NewVChareMsg); 
      type.chare.forAnyPe = f; 
    }
    void*  getVidPtr(void) const {
      CkAssert(getMsgtype()==NewVChareMsg || getMsgtype()==ForVidMsg
          || getMsgtype()==FillVidMsg);
      return type.chare.ptr;
    }
    void   setVidPtr(void *p) {
      CkAssert(getMsgtype()==NewVChareMsg || getMsgtype()==ForVidMsg
          || getMsgtype()==FillVidMsg);
      type.chare.ptr = p;
    }
    void*  getObjPtr(void) const { 
      CkAssert(getMsgtype()==ForChareMsg); return type.chare.ptr; 
    }
    void   setObjPtr(void *p) { 
      CkAssert(getMsgtype()==ForChareMsg); type.chare.ptr = p; 
    }

// Group-specific fields
    CkGroupID   getGroupNum(void) const {
      CkAssert(getMsgtype()==BocInitMsg || getMsgtype()==ForBocMsg
          || getMsgtype()==NodeBocInitMsg || getMsgtype()==ForNodeBocMsg);
      return type.group.g;
    }
    void   setGroupNum(const CkGroupID g) {
      CkAssert(getMsgtype()==BocInitMsg || getMsgtype()==ForBocMsg
          || getMsgtype()==NodeBocInitMsg || getMsgtype()==ForNodeBocMsg);
      type.group.g = g;
    }
    void setGroupEpoch(int epoch) { type.group.epoch=epoch; }
    int getGroupEpoch(void) { return type.group.epoch; }
    void setRednMgr(CkNodeGroupID r){ type.group.rednMgr = r; }
    CkNodeGroupID getRednMgr(){ return type.group.rednMgr; }

// Array-specific fields
    CkGroupID &getsetArrayMgr(void) {return type.array.arr;}
    UShort &getsetArrayEp(void) {return epIdx;}
    UShort &getsetArrayBcastEp(void) {return type.group.arrayEp;}
    UInt &getsetArraySrcPe(void) {return pe;}
    UChar &getsetArrayHops(void) {return type.array.hopCount;}
    int getArrayIfNotThere(void) {return type.array.ifNotThere;}
    void setArrayIfNotThere(int nt) {type.array.ifNotThere=nt;}
    int *getsetArrayListenerData(void) {return type.array.listenerData;}
    CkArrayIndexMax &getsetArrayIndex(void) 
    	{return *(CkArrayIndexMax *)&type.array.index;}
};

inline envelope *UsrToEnv(const void *const msg) {
  return (((envelope *) msg)-1);
}

inline void *EnvToUsr(const envelope *const env) {
  return ((void *)(env+1));
}

inline envelope *_allocEnv(const int msgtype, const int size=0, const int prio=0) {
  return envelope::alloc(msgtype,size,prio);
}

inline void *_allocMsg(const int msgtype, const int size, const int prio=0) {
  return EnvToUsr(envelope::alloc(msgtype,size,prio));
}

extern UChar   _defaultQueueing;

extern void CkPackMessage(envelope **pEnv);
extern void CkUnpackMessage(envelope **pEnv);

class MsgPool: public SafePool<void *> {
private:
    static void *_alloc(void) {
      register envelope *env = _allocEnv(ForChareMsg,0,0);
      env->setQueueing(_defaultQueueing);
      env->setMsgIdx(0);
      return EnvToUsr(env);
    }
public:
    MsgPool():SafePool<void*>(_alloc, CkFreeMsg) {}
};

CkpvExtern(MsgPool*, _msgPool);

#endif
