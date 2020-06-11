﻿#include "stdafx.h"
#include "WebCommandMgr.h"
#include "GameService.h"
#include "../Message/Msg_ID.pb.h"
#include "../Message/Game_Define.pb.h"
#include "PacketHeader.h"
#include "HttpParameter.h"
#include "../Message/Msg_Game.pb.h"

CWebCommandMgr::CWebCommandMgr()
{

}

CWebCommandMgr::~CWebCommandMgr()
{

}

CWebCommandMgr* CWebCommandMgr::GetInstancePtr()
{
	static CWebCommandMgr _StaticMgr;

	return &_StaticMgr;
}


BOOL CWebCommandMgr::Init()
{
	return TRUE;
}

BOOL CWebCommandMgr::Uninit()
{
	return TRUE;
}

BOOL CWebCommandMgr::SendWebResult(UINT32 nConnID, EWebResult eResult)
{
	std::string strResult = CommonConvert::IntToString((INT64)eResult);

	ServiceBase::GetInstancePtr()->SendMsgRawData(nConnID, MSG_PHP_GM_COMMAND_ACK, 0, 0, strResult.c_str(), (UINT32)strResult.size());

	return TRUE;
}

BOOL CWebCommandMgr::DispatchPacket(NetPacket* pNetPacket)
{
	switch (pNetPacket->m_dwMsgID)
	{
			PROCESS_MESSAGE_ITEM(MSG_PHP_GM_COMMAND_REQ, OnMsgGmCommandReq);
			PROCESS_MESSAGE_ITEM(MSG_SEAL_ACCOUNT_ACK, OnMsgSealAccountAck);
	}


	return FALSE;
}

BOOL CWebCommandMgr::OnMsgGmCommandReq(NetPacket* pNetPacket)
{
	CHAR szMsgBuf[1024] = { 0 };
	strncpy(szMsgBuf, pNetPacket->m_pDataBuffer->GetData(), pNetPacket->m_pDataBuffer->GetBodyLenth());

	HttpParameter Params;
	Params.ParseStringToMap(szMsgBuf);
	std::string strAction = Params.GetStrValue("Action");
	CLog::GetInstancePtr()->LogError("GmCommand Event:%s", strAction.c_str());

	EWebAction eWebAction = (EWebAction)CommonConvert::StringToInt(strAction.c_str());

	switch (eWebAction)
	{
		case EWA_SEAL_ACCOUNT:
			OnGmSealAccount(Params, pNetPacket->m_dwConnID);
			break;
		case EWA_SERVER_CHNAGE:
			OnGmServerChange(Params, pNetPacket->m_dwConnID);
		case  EWA_REVIEW_CHANGE:
			OnGmReviewChange(Params, pNetPacket->m_dwConnID);
		default:
			SendWebResult(pNetPacket->m_dwConnID, EWR_INVALID_ACT);
			break;
	}

	return TRUE;
}

BOOL CWebCommandMgr::OnMsgSealAccountAck(NetPacket* pPacket)
{
	SealAccountAck Ack;
	Ack.ParsePartialFromArray(pPacket->m_pDataBuffer->GetData(), pPacket->m_pDataBuffer->GetBodyLenth());
	PacketHeader* pHeader = (PacketHeader*)pPacket->m_pDataBuffer->GetBuffer();

	UINT32 nConnID = pHeader->dwUserData;
	ERROR_RETURN_TRUE(nConnID != 0);

	SendWebResult(nConnID, EWR_SUCCESSED);

	return TRUE;
}

void CWebCommandMgr::OnGmSealAccount(HttpParameter& hParams, UINT32 nConnID)
{
	ERROR_RETURN_NONE(nConnID != 0);
	SealAccountReq Req;
	Req.set_accountid(hParams.GetLongValue("accountid"));
	Req.set_accountname(hParams.GetStrValue("accountname"));
	Req.set_channel(hParams.GetIntValue("channel"));
	Req.set_seal(hParams.GetIntValue("seal"));
	Req.set_sealtime(hParams.GetIntValue("sealtime"));
	CGameService::GetInstancePtr()->SendCmdToAccountConnection(MSG_SEAL_ACCOUNT_REQ, nConnID, 0, Req);
	return;
}

void CWebCommandMgr::OnGmServerChange(HttpParameter& hParams, UINT32 nConnID)
{
	INT32 nSvrID = hParams.GetIntValue("serverid");
	CGameService::GetInstancePtr()->m_LoginMsgHandler.m_LogicSvrMgr.ReloadServerList(nSvrID);
	SendWebResult(nConnID, EWR_SUCCESSED);
	return;
}

void CWebCommandMgr::OnGmReviewChange(HttpParameter& hParams, UINT32 nConnID)
{
	CGameService::GetInstancePtr()->m_LoginMsgHandler.m_LogicSvrMgr.ReloadReviewVersion();
	SendWebResult(nConnID, EWR_SUCCESSED);
	return;
}
