#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "sqrdbg.h"
#include "sqdbgserver.h"

SQInteger debug_hook(HSQUIRRELVM v);
SQInteger error_handler(HSQUIRRELVM v);

#include "serialize_state.inl"
typedef SQDbgServer* HSQREMOTEDBG;

HSQREMOTEDBG sq_rdbg_init(HSQUIRRELVM v,unsigned short port,SQBool autoupdate)
{
	sockaddr_in bindaddr;
	SQDbgServer *rdbg = new SQDbgServer(v);
	rdbg->_autoupdate = autoupdate?true:false;
	rdbg->_accept = socket(AF_INET,SOCK_STREAM,0);
    int opt = 1;
//    setsockopt(rdbg->_accept, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
//               &opt, sizeof(opt));

	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	bindaddr.sin_addr.s_addr = INADDR_ANY;

	if(bind(rdbg->_accept,(sockaddr*)&bindaddr,sizeof(bindaddr))==-1){
		delete rdbg;
		sq_throwerror(v,_SC("failed to bind the socket"));
		return NULL;
	}
	if(!rdbg->Init()) {
		delete rdbg;
		sq_throwerror(v,_SC("failed to initialize the debugger"));
		return NULL;
	}
	
    return rdbg;
}

SQRESULT sq_rdbg_waitforconnections(HSQREMOTEDBG rdbg)
{
    SQRESULT i = sq_compilebuffer(rdbg->_v, serialize_state_nut, sizeof(serialize_state_nut), _SC("SERIALIZE_STATE"), SQTrue);
    if(SQ_FAILED(i)) {
		sq_throwerror(rdbg->_v,_SC("error compiling the serialization function"));
        return SQ_ERROR;
	}
	sq_getstackobj(rdbg->_v,-1,&rdbg->_serializefunc);
	sq_addref(rdbg->_v,&rdbg->_serializefunc);
	sq_pop(rdbg->_v,1);

    struct sockaddr_in address;
    int addrlen = sizeof(address);
	if(listen(rdbg->_accept,1)==-1)
		return sq_throwerror(rdbg->_v,_SC("error on listen(socket)"));
	rdbg->_endpoint = accept(rdbg->_accept, (struct sockaddr *)&address, (socklen_t*)&addrlen);
	if(rdbg->_endpoint==-1){
		return sq_throwerror(rdbg->_v,_SC("error accept(socket)"));
	}
	while(!rdbg->_ready){
		sq_rdbg_update(rdbg);
	}
	return SQ_OK;
}


SQRESULT sq_rdbg_update(HSQREMOTEDBG rdbg)
{

    timeval time;
	time.tv_sec=0;
	time.tv_usec=0;
	fd_set read_flags;
    FD_ZERO(&read_flags);
	FD_SET(rdbg->_endpoint, &read_flags);
	select(NULL/*ignored*/, &read_flags, NULL, NULL, &time);

	if(FD_ISSET(rdbg->_endpoint,&read_flags)){
		char temp[1024];
		int size=0;
		char c,prev=NULL;
		memset(&temp,0,sizeof(temp));
		int res;
		FD_CLR(rdbg->_endpoint, &read_flags);
		while((res = recv(rdbg->_endpoint,&c,1,0))>0){
			
			if(c=='\n')break;
			if(c!='\r'){
				temp[size]=c;
				prev=c;
				size++;
			}
			if(size >= sizeof(temp)-2) break;
		}
		switch(res){

		case 0:
			return sq_throwerror(rdbg->_v,_SC("disconnected"));
		case -1:
			return sq_throwerror(rdbg->_v,_SC("socket error"));
        }
		
		temp[size]=NULL;
		temp[size+1]=NULL;
		rdbg->ParseMsg(temp);
	}
	return SQ_OK;
}

SQInteger debug_hook(HSQUIRRELVM v)
{
	SQUserPointer up;
	SQInteger event_type,line;
	const SQChar *src,*func;
	sq_getinteger(v,2,&event_type);
	sq_getstring(v,3,&src);
	sq_getinteger(v,4,&line);
	sq_getstring(v,5,&func);
	sq_getuserpointer(v,-1,&up);
	HSQREMOTEDBG rdbg = (HSQREMOTEDBG)up;
	rdbg->Hook(v,event_type,line,src,func);
	if(rdbg->_autoupdate) {
		if(SQ_FAILED(sq_rdbg_update(rdbg)))
			return sq_throwerror(v,_SC("socket failed"));
	}
	return 0;
}

SQInteger error_handler(HSQUIRRELVM v)
{
	SQUserPointer up;
	const SQChar *sErr=NULL;
	const SQChar *fn=_SC("unknown");
	const SQChar *src=_SC("unknown");
	SQInteger line=-1;
	SQStackInfos si;
	sq_getuserpointer(v,-1,&up);
	HSQREMOTEDBG rdbg=(HSQREMOTEDBG)up;
	if(SQ_SUCCEEDED(sq_stackinfos(v,1,&si)))
	{
		if(si.funcname)fn=si.funcname;
		if(si.source)src=si.source;
		line=si.line;
		scprintf(_SC("*FUNCTION [%s] %s line [%d]\n"),fn,src,si.line);
	}
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
			scprintf(_SC("\nAN ERROR HAS OCCURED [%s]\n"),sErr);
			rdbg->Break(v,si.line,src,_SC("error"),sErr);
		}
		else{
			scprintf(_SC("\nAN ERROR HAS OCCURED [unknown]\n"));
			rdbg->Break(v,si.line,src,_SC("error"),_SC("unknown"));
		}
	}
	rdbg->BreakExecution();
	return 0;
}


SQRESULT sq_rdbg_shutdown(HSQREMOTEDBG rdbg)
{
	delete rdbg;
	return SQ_OK;
}
