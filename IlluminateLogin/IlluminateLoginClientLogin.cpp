#include <iostream>
#include <string>
#include <sstream>
using std::cout;
using std::endl;
using std::string;
using std::stringstream;

#include "AlefLoginGlobal.h"
#include "AlefLoginServerSystems.h"
#include "AlefLoginClientLogin.h"
#include "AlefHash.h"

AlefLoginClientLogin::AlefLoginClientLogin()
{

}

AlefLoginClientLogin::~AlefLoginClientLogin()
{

}

bool AlefLoginClientLogin::processPacket(const localInfo& local)
{
	/*AuPacket::GetField(
		v10,
		1,
		pvPacket,
		nSize,
		&pvFirst,
		&lResult,
		&pvCharDetailInfo,
		&lCID,
		&cAccountLen,
		&pszPassword,
		&lIsProtected,
		&pszAccountID,
		&pszExtraForForeign,
		&pszEncryptCode,
		&v35,
		&cPasswordLen,
		&pvPacketVersionInfo,
		&pvDetailServerInfo,
		&pszChallenge,
		&lIsLimited);*/
	/*  GetField(_param_3,(int)&this->m_pAgpmLogin->m_csPacket,&DAT_00000001,(short)param_2,_param_3,
           (int)&pszAccountID + 3,&uStack8,&pszExtraForForeign,&lIsProtected,&lCID,&cAccountLen,
           &lIsLimited,&pvCharDetailInfo,&pvPacketVersionInfo,&lResult,auStack4,&pszPassword,
           &cPasswordLen,&pszEncryptCode,&pvDetailServerInfo,&pszChallenge);*/
	/*Alef::INT8, Alef::CHAR, Alef::CHAR, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT32, Alef::CHAR, Alef::PACKET, Alef::PACKET, Alef::INT32, Alef::PACKET, Alef::CHAR, Alef::CHAR, Alef::INT32, Alef::INT32*/
	/* 1, 32, 49, 1, 33, 1, 1, 32, 1, 1, 1, 1, 2049, 5, 1, 1*/
	localInfo& localObj = const_cast<localInfo&>(local);
	AlefPacket* packet = localObj.packet;
	Int8 i8Operation = 0;
	//char cKey[32] = { 0 }, cAcct[49] = { 0 }, cPW[33] = { 0 }, cUnk1[32] = { 0 }, cUnk2[2049] = { 0 }, cUnk3[5] = { 0 };
	string cKey = "", cAcct = "", cPW = "", cUnk1 = "", cUnk2 = "", cUnk3 = "";
	cKey.reserve(32);
	cAcct.reserve(49);
	cPW.reserve(33);
	cUnk1.reserve(32);
	cUnk2.reserve(2049);
	cUnk3.reserve(5);
	UInt8 ui8AcctLen = 0, ui8PWLen = 0;
	Int32 i32Unk1 = 0, i32Unk2 = 0, i32IsLimited = 0, i32IsProtected = 0;
	SharedPtr<AlefPacket> packets[3] = { new AlefPacket(), new AlefPacket(), new AlefPacket() };

	pktInterface->processPacket(packet, &i8Operation, cKey.c_str(), cAcct.c_str(), &ui8AcctLen, cPW.c_str(), &ui8PWLen, &i32Unk1, cUnk1.c_str(), &packets[0], &packets[1], &i32Unk2, &packets[2], cUnk2.c_str(), cUnk3.c_str(), &i32IsLimited, &i32IsProtected);
	switch(i8Operation)
	{
		case CLIENTLOGIN_HASHKEY: //Initial Packet Opcode
			return processHashKeyPacket(localObj); break;
		case CLIENTLOGIN_USERLOGIN: //User Login Packet Opcode
			return processUserLoginPacket(localObj, cAcct.c_str(), ui8AcctLen, cPW.c_str(), ui8PWLen); break;
		case CLIENTLOGIN_UNIONINFO: //User World Union Info
			return processUnionInfo(localObj); break;
		case CLIENTLOGIN_CHARLIST: //User Character List Request
			return processCharacterList(localObj); break;
		case CLIENTLOGIN_WORLDENTER: //World Enter
			return processWorldEnterRequest(localObj); break;
		case CLIENTLOGIN_CHARCREATION: //Character Creation Request
			return processCharacterCreation(localObj); break;
		default:
		{
			stringstream errorMsg;
			errorMsg << "User Connect processPacket Unhandled Operation: " << (int)i8Operation;
			LOG(errorMsg.str(), FATAL);
			return false;
		}
	}
	return false;
}

bool AlefLoginClientLogin::processHashKeyPacket(localInfo& local)
{
	LOG("processHashKeyPacket");

	//Can check for version information here

	Int8 i8Operation = CLIENTLOGIN_HASHKEY;

	string hashKey = _localSys->_localAcct()->generateHashKey();

	_localSys->_localAcct()->setLoginStep(HASHKEY);

	SharedPtr<AlefPacket> response = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, hashKey.c_str(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(response);

	return true;
}

bool AlefLoginClientLogin::processUserLoginPacket(localInfo& local, const char* acct, UInt8 acctLen, const char* pw, UInt8 pwLen)
{
	LOG("processUserLoginPacket");
	string user = acct, pass = pw;

	if (!_localSys->_localAcct()->decryptUserInfo(user, acctLen, pass, pwLen))
	{
		LOG("processUserLoginPacket: decryptUserInfo FAIL!", FATAL);
		return false;
	}

	string hashPW = AlefHash::getHash(pass);

	if (!serverLoginSys->checkLogin(user, hashPW))
	{
		sendLoginResult(_localSock, INCORRECTUSER); //Incorrect User ID
		return true;
	}

	_localSys->_localAcct()->setLoginStep(AUTHENTICATED);

	//get character data
	//serverLoginSys->getCharData(_localSys->_localChar());

	Int8 i8Operation = CLIENTLOGIN_USERLOGIN;
	Int32 i32IsLimited = 0, i32IsProtected = 0;
	SharedPtr<AlefPacket> signOnResponse = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, acct, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &i32IsLimited, &i32IsProtected);

	_localSock.sendPacket(signOnResponse);

	return true;
}

bool AlefLoginClientLogin::processUnionInfo(localInfo& local)
{
	int unionType = 0;
	//Alef::INT32, Alef::CHAR, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::CHAR
	SharedPtr<AlefPacket> miniCharInfo = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_CHAR_INFO, 0, 0, 0, 0, &unionType, 0, 0, 0, 0);

	/*Alef::INT8, Alef::CHAR, Alef::CHAR, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT32, Alef::CHAR, Alef::PACKET, Alef::PACKET, Alef::INT32, Alef::PACKET, Alef::CHAR, Alef::CHAR, Alef::INT32, Alef::INT32*/
	Int8 i8Operation = 3;
	SharedPtr<AlefPacket> unionResponse = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, 0, 0, 0, 0, 0, 0, &miniCharInfo, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(unionResponse);

	unsigned char name[] = "test";
	int maxChars = 1, index = 0;

	SharedPtr<AlefPacket> charName = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_CHAR_INFO, 0, name, &maxChars, &index, 0, 0, 0, 0, 0);

	i8Operation = 4;

	SharedPtr<AlefPacket> charNameResponse = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, 0, 0, 0, 0, 0, 0, &charName, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(charNameResponse);

	i8Operation = 5;
	unsigned char acct[] = "acct";

	SharedPtr<AlefPacket> charNameFinish = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, acct, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(charNameFinish);

	return true;
}

bool AlefLoginClientLogin::processCharacterList(localInfo& local)
{
	_localSys->_localAcct()->setLoginStep(CHARLIST);
	sendDummyCharacter(_localSock);

	Int8 i8Operation = 7;
	Int32 cID = 0;
	unsigned char acct[] = "acct";

	SharedPtr<AlefPacket> sendCharInfoFinish = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, acct, 0, 0, 0, &cID, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(sendCharInfoFinish);

	return true;
}

bool AlefLoginClientLogin::processCharacterCreation(localInfo& local)
{
	_localSys->_localAcct()->setLoginStep(CHARCREATE);

	if (!sendBaseCharPackets(_localSock))
		return false;

	//{	Alef::INT32, Alef::CHAR, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::CHAR }
	Int32 numChar = 15;
	Int32 race = 1;
	SharedPtr<AlefPacket> loginCharInfo = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_CHAR_INFO, 0, 0, &numChar, 0, 0, &race, 0, 0, 0);

	Int8 i8Operation = 10;
	SharedPtr<AlefPacket> sendCharCreateFinish = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, 0, 0, 0, 0, 0, 0, &loginCharInfo, 0, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(sendCharCreateFinish);

	return true;
}

bool AlefLoginClientLogin::processWorldEnterRequest(localInfo& local)
{
	LOG("World Enter Request");
	_localSys->_localAcct()->setLoginStep(ENTERWORLD);

	//{	Alef::INT8, Alef::CHAR, Alef::INT32, Alef::CHAR, Alef::INT32, Alef::VEC3F, Alef::INT32 }
	Int8 i8Operation = 11;
	Int32 authKey = 12345; //What sorta number is expected here?
	unsigned char name[] = "Dummy#test";
	SharedPtr<AlefPacket> authKeyPkt = pktInterface->buildPacket(Alef::AGSMCHARMANAGER_PACKET_TYPE, &i8Operation, 0, 0, name, 0, 0, &authKey);

	_localSock.sendPacket(authKeyPkt);

	//{Alef::INT32, Alef::CHAR, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::CHAR}
	Int32 i32TID = 96;
	memset(name, 0, 10);//clear name
	strcpy((char*)name, "test");
	SharedPtr<AlefPacket> miniCharInfo = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_CHAR_INFO, &i32TID, name, 0, 0, 0, 0, 0, 0, 0);

	//TODO: acquire serverAddress from serverListSys
	//{	Alef::CHAR }
	std::string serverAddress = loginConfig->getWorldAddress();
	
	SharedPtr<AlefPacket> miniServerInfo = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_SERVER_INFO, serverAddress.c_str());

	//{	Alef::INT8, Alef::CHAR, Alef::CHAR, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT32, Alef::CHAR, Alef::PACKET, Alef::PACKET, Alef::INT32, Alef::PACKET, Alef::CHAR, Alef::CHAR, Alef::INT32, Alef::INT32}
	i8Operation = 8;
	Int32 i32CID = 0;
	SharedPtr<AlefPacket> serverInfoPkt = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, 0, 0, 0, 0, &i32CID, 0, miniCharInfo, miniServerInfo, 0, 0, 0, 0, 0, 0);

	_localSock.sendPacket(serverInfoPkt);

	return true;
}

void AlefLoginClientLogin::sendDummyCharacter(AlefSocket& sock)
{
	Int32 i32Dummy = 100, i32One = 1, i32Zero = 0;
	float fZero = 0;
	Int8 i8Zero = 0;

	//AlefCharacter char = AlefServerCharacterSys->getCharacterData();

	//{	Alef::VEC3F, Alef::VEC3F, Alef::INT32, Alef::INT32, Alef::FLOAT, Alef::FLOAT, Alef::INT8, Alef::INT8 }
	Alef::AlefVec3F pos, destPos;
	SharedPtr<AlefPacket> charMovePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_MOVE, &pos, &destPos, &i32Zero, &i32Zero, &fZero, &fZero, &i8Zero, &i8Zero);
	//SharedPtr<AlefPacket> charMovePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_MOVE, &char.Pos, &char.destPos, &i32Zero, &i32Zero, &fZero, &fZero, &i8Zero, &i8Zero);

#pragma region FACTORPACKETS

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorStatusPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_STATUS, &i32Dummy, &i32Dummy, &i32Dummy, &i32Dummy, &i32Dummy, 0, &i32Dummy, &i32Dummy, &i32Dummy, &i32Dummy, 0, &i32Zero, 0);

	//{	Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorTypePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_TYPE, &i32One, &i32One, &i32One);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32}
	SharedPtr<AlefPacket> factorPointPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINT, &i32Dummy, &i32Dummy, &i32Dummy, &i32Dummy, &i32Dummy, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorPointMaxPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINTMAX, &i32Dummy, &i32Dummy, &i32Zero, &i32Zero, &i32Zero, 0, 0, 0, 0, 0, 0);

	//Alef::AGPMCHAR_FACTOR_RECOVERY Skipped

	//Alef::AGPMCHAR_FACTOR_ATTRIBUTE Skipped

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorAttributeEmpty = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDmgPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DMG, factorAttributeEmpty, factorAttributeEmpty);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDefPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DEFENSE, factorAttributeEmpty, factorAttributeEmpty);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	Int32 i32AtkRng = 50, i32AtkSpd = 60;
	SharedPtr<AlefPacket> factorAttackPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTACK, &i32Dummy, &i32AtkRng, &i32AtkSpd, 0, 0, 0, 0, 0);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorDirtPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DIRT, 0, 0, 0, 0, 0, &i32Zero, 0, &i32Zero);

	//Alef::AGPMCHAR_FACTOR_PRICE Skipped

	//{	Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorResPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTORS, 0, factorStatusPkt, factorTypePkt, 0, factorPointMaxPkt, 0, 0, factorDmgPkt, factorDefPkt, factorAttackPkt, 0, factorDirtPkt, 0);

	//{	Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTORS, factorResPkt, factorStatusPkt, factorTypePkt, factorPointPkt, factorPointMaxPkt, 0, 0, factorDmgPkt, factorDefPkt, factorAttackPkt, 0, factorDirtPkt, 0);

#pragma endregion	

	Int8 i8Operation = 0;
	Int16 i16Zero = 0;
	Int32 charID = 1012, charTID = 6, nameLen = 4;
	Int64 i64Zero = 0;
	unsigned char charName[] = "test", skillInit[33] = { 0 }, signature[] = "Signature";
	/*{	Alef::INT8, Alef::INT32, Alef::INT32, Alef::MEMORY_BLOCK, Alef::INT8, Alef::PACKET, Alef::PACKET, Alef::PACKET,
	Alef::INT64, Alef::INT64, Alef::INT64, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT8, Alef::UINT8,
	Alef::UINT64, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT16, Alef::INT32,
	Alef::INT32, Alef::CHAR, Alef::MEMORY_BLOCK, Alef::UINT32 }*/
	SharedPtr<AlefPacket> charInfoPkt = pktInterface->buildPacket(Alef::AGPMCHARACTER_PACKET_TYPE,	&i8Operation, &charID, &charTID, &nameLen, charName, &i8Zero, charMovePkt, 0, factorPkt, &i64Zero, &i64Zero, &i64Zero, &i8Zero,
																							&i8Zero, &i32Zero, &i8Zero, &i8Zero, &i8Zero, &i64Zero, &i8Zero, skillInit, &i8Zero, &i8Zero, &i32Zero, &i8Zero, &i16Zero, &i32Zero,
																							&i32Zero, signature, &i16Zero, 0, &i32Zero);

	sock.sendPacket(charInfoPkt);
}

void AlefLoginClientLogin::sendLoginResult(AlefSocket& sock, int loginStatus)
{
	/*v4 = this;
  v5 = &this->m_pAgpmLogin->m_csPacketCharInfo;
  v10 = 15;
  v6 = AuPacket::MakePacket(v5, 0, (__int16 *)&pszCharName, 13, 0, pszCharName, 0, 0, 0, 0, 0, 0, 0);
  v7 = AuPacket::MakePacket(
         &v4->m_pAgpmLogin->m_csPacket,
         1,
         (__int16 *)&pszCharName,
         13,
         &v10,
         0,
         0,
         0,
         0,
         0,
         0,
         0,
         v6,
         0,
         &lResult,
         0,
         0,
         0,
         0,
         0);*/

	SharedPtr<AlefPacket> charName = pktInterface->buildMiniPacket(Alef::AGPMLOGIN_CHAR_INFO, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	Int8 i8Operation = CLIENTLOGIN_LOGINRESULT;

	SharedPtr<AlefPacket> loginResult = pktInterface->buildPacket(Alef::AGPMLOGIN_PACKET_TYPE, &i8Operation, 0, 0, 0, 0, 0, 0, 0, charName, 0, &loginStatus, 0, 0, 0, 0, 0);

	sock.sendPacket(loginResult);
}

bool AlefLoginClientLogin::sendBaseCharPackets(AlefSocket& sock)
{
	SharedPtr<AlefPacket> baseChar = getBaseCharPacket(BASECHAR::KNIGHT);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::ARCHER);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::MAGE);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::ZERKER);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::HUNTER);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::SORC);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::SWASH);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::RANGER);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::ELEMENTALIST);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::SCION);

	sock.sendPacket(baseChar);

	/*baseChar = getBaseCharPacket(BASECHAR::SLAYER);

	sock.sendPacket(baseChar);

	baseChar = getBaseCharPacket(BASECHAR::ORBITER);

	sock.sendPacket(baseChar);*/

	return true;
}


SharedPtr<AlefPacket> AlefLoginClientLogin::getBaseCharPacket(BASECHAR base)
{
	SharedPtr<CharDataInfo> baseChar;

	baseChar = serverDataSys->getCharDataFromDBID(static_cast<UInt32>(base));

	if (!baseChar)
		return nullptr;

	CharacterData charData;

	charData.charTID = baseChar->tID;
	charData.charName = baseChar->name;

	return buildCharPacket(charData);
}

/*struct CharacterData
{
	string charName;
	Int32 charID;
	Int32 charTID; //Template ID
	CharacterMove charMove;
	CharacterFactors charFactors;
	UInt64 money;
	UInt64 bankMoney;
	UInt64 cashMoney;
	Int8 actionStatus;
	Int8 criminalStatus;
	Int8 face;
	Int8 hair;
	Int32 optionFlag;
	Int8 bankSize;
	UInt32 remainingCrimTime;
	UInt32 remainingMurderTime;
	string signature;
};

{	Alef::INT8, Alef::INT32, Alef::INT32, Alef::MEMORY_BLOCK, Alef::INT8, Alef::PACKET, Alef::PACKET, Alef::PACKET,
	Alef::INT64, Alef::INT64, Alef::INT64, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT8, Alef::UINT8,
	Alef::UINT64, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT16, Alef::INT32,
	Alef::INT32, Alef::CHAR, Alef::MEMORY_BLOCK, Alef::UINT32 }
SharedPtr<AlefPacket> charInfoPkt = pktInterface->buildPacket(Alef::AGPMCHARACTER_PACKET_TYPE, &i8Operation, &charID, &charTID, &nameLen, charName, &i8Zero, charMovePkt, 0, factorPkt, &i64Zero, &i64Zero, &i64Zero, &i8Zero,
	&i8Zero, &i32Zero, &i8Zero, &i8Zero, &i8Zero, &i64Zero, &i8Zero, skillInit, &i8Zero, &i8Zero, &i32Zero, &i8Zero, &i16Zero, &i32Zero,
	&i32Zero, signature, &i16Zero, 0, &i32Zero);
*/


SharedPtr<AlefPacket> AlefLoginClientLogin::buildCharPacket(CharacterData& data)
{
	CharacterMove charMove = data.charMove;
	CharacterFactors charFactors = data.charFactors;

	//AlefCharacter char = AlefServerCharacterSys->getCharacterData();

	SharedPtr<AlefPacket> charMovePkt = buildCharMovePacket(charMove);
	SharedPtr<AlefPacket> charActionPkt = nullptr;
	SharedPtr<AlefPacket> factorPkt = buildFactorPacket(charFactors);

	Int32 nameLen = data.charName.length();
	Int8 i8Operation = 0;
	Int16 i16Zero = 0;
	/*Int32 charID = 1012, charTID = 6, nameLen = 4;
	Int64 i64Zero = 0;
	unsigned char charName[] = "test", skillInit[33] = { 0 }, signature[] = "Signature";*/
	/*{	Alef::INT8, Alef::INT32, Alef::INT32, Alef::MEMORY_BLOCK, Alef::INT8, Alef::PACKET, Alef::PACKET, Alef::PACKET,
	Alef::INT64, Alef::INT64, Alef::INT64, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT8, Alef::UINT8,
	Alef::UINT64, Alef::INT8, Alef::CHAR, Alef::INT8, Alef::INT8, Alef::INT32, Alef::INT8, Alef::UINT16, Alef::INT32,
	Alef::INT32, Alef::CHAR, Alef::MEMORY_BLOCK, Alef::UINT32 }*/
	SharedPtr<AlefPacket> charInfoPkt = pktInterface->buildPacket(Alef::AGPMCHARACTER_PACKET_TYPE, &i8Operation, &data.charSessionID, &data.charTID, &nameLen, data.charName.c_str(), &data.charStatus, charMovePkt, /*charActionPkt*/0, factorPkt,
		&data.money, &data.bankMoney, &data.cashMoney, &data.actionStatus, &data.criminalStatus, &data.attackerID, &data.isNewChar, &data.lastBindingIdx, &data.socialAnimIdx, &data.specialStatus, &data.transformStatus, data.skillInit.c_str(), 
		&data.face, &data.hair, &data.optionFlag, &data.bankSize, &data.eventStatusFlag, &data.remainingCrimTime, &data.remainingMurderTime, data.signature.c_str(), &data.GGSize, 0, &data.lastKilledBSquare);

	return charInfoPkt;
}

SharedPtr<AlefPacket> AlefLoginClientLogin::buildCharMovePacket(CharacterMove& move)
{
	SharedPtr<AlefPacket> charMovePkt;

	//{	Alef::VEC3F, Alef::VEC3F, Alef::INT32, Alef::INT32, Alef::FLOAT, Alef::FLOAT, Alef::INT8, Alef::INT8 }
	charMovePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_MOVE, &move.curPos, &move.destPos, &move.followTargetId, &move.followDistance, &move.axisX, &move.axisY, &move.flags, &move.direction);

	return charMovePkt;
}

SharedPtr<AlefPacket> AlefLoginClientLogin::buildFactorPacket(CharacterFactors& factors)
{
	CharacterFactorStatus charStatus = factors.status;
	CharacterFactorType charType = factors.type;
	CharacterFactorPoint charPoint = factors.point;
	CharacterFactorPointMax charPointMax = factors.pointMax;
	CharacterFactorRecovery charRecovery = factors.recovery;
	CharacterFactorAttribute charAttribute = factors.attribute;
	CharacterFactorDamage charDmg = factors.damage;
	CharacterFactorDefense charDef = factors.defense;
	CharacterFactorAttack charAtk = factors.attack;
	CharacterFactorDirt charDirt = factors.dirt;
	CharacterFactorPrice charPrice = factors.price;
	CharacterFactorItem charItem = factors.item;
	CharacterFactorResult charRes = factors.result;

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorStatusPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_STATUS, 
		&charStatus.stamina, &charStatus.strength, &charStatus.intelligence, &charStatus.dexterity, &charStatus.charisma, &charStatus.luck, 
		&charStatus.wisdom, &charStatus.level, &charStatus.moveSpeed, &charStatus.runSpeed, &charStatus.unionRank, &charStatus.villainPoints, &charStatus.unk);

	//{	Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorTypePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_TYPE, &charType.charRace, &charType.charGender, &charType.charClass);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32}
	SharedPtr<AlefPacket> factorPointPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINT, &charPoint.hp, &charPoint.mana, &charPoint.skillPoints, &charPoint.xpLow, 
		&charPoint.xpHigh, &charPoint.AP, &charPoint.MAP, &charPoint.MI, &charPoint.agro, &charPoint.damage, &charPoint.damageMagic, &charPoint.damageWater, &charPoint.damageFire, 
		&charPoint.damageEarth, &charPoint.damageAir, &charPoint.damagePoison, &charPoint.damageLightning, &charPoint.damageIce, &charPoint.bonusXP, &charPoint.damageHeroic);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorPointMaxPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINTMAX, &charPointMax.maxHP, &charPointMax.maxMana, &charPointMax.maxSkillPoints, 
		&charPointMax.xpLow, &charPointMax.xpHigh, &charPointMax.AP, &charPointMax.MAP, &charPointMax.MI, &charPointMax.AR, &charPointMax.DR, &charPointMax.MAR);

	//Alef::AGPMCHAR_FACTOR_RECOVERY Skipped
	SharedPtr<AlefPacket> factorRecoveryPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_RECOVERY, &charRecovery.hpRecover, &charRecovery.manaRecovery, &charRecovery.spRecovery);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorAttributePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charAttribute.physical, &charAttribute.magic, &charAttribute.water, &charAttribute.fire,
		&charAttribute.earth, &charAttribute.air, &charAttribute.poison, &charAttribute.lightning, &charAttribute.ice, &charAttribute.physBlock, &charAttribute.skillBlock, &charAttribute.heroic,
		&charAttribute.heroicMelee, &charAttribute.heroicRange, &charAttribute.heroicMagic);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDmgMinPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDmg.min.physical, &charDmg.min.magic, &charDmg.min.water, &charDmg.min.fire,
		&charDmg.min.earth, &charDmg.min.air, &charDmg.min.poison, &charDmg.min.lightning, &charDmg.min.ice, &charDmg.min.physBlock, &charDmg.min.skillBlock, &charDmg.min.heroic,
		&charDmg.min.heroicMelee, &charDmg.min.heroicRange, &charDmg.min.heroicMagic);

	SharedPtr<AlefPacket> factorDmgMaxPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDmg.max.physical, &charDmg.max.magic, &charDmg.max.water, &charDmg.max.fire,
		&charDmg.max.earth, &charDmg.max.air, &charDmg.max.poison, &charDmg.max.lightning, &charDmg.max.ice, &charDmg.max.physBlock, &charDmg.max.skillBlock, &charDmg.max.heroic,
		&charDmg.max.heroicMelee, &charDmg.max.heroicRange, &charDmg.max.heroicMagic);

	SharedPtr<AlefPacket> factorDmgPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DMG, factorDmgMinPkt, factorDmgMaxPkt);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDefPointPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDef.point.physical, &charDef.point.magic, &charDef.point.water, &charDef.point.fire,
		&charDef.point.earth, &charDef.point.air, &charDef.point.poison, &charDef.point.lightning, &charDef.point.ice, &charDef.point.physBlock, &charDef.point.skillBlock, &charDef.point.heroic,
		&charDef.point.heroicMelee, &charDef.point.heroicRange, &charDef.point.heroicMagic);

	SharedPtr<AlefPacket> factorDefRatePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDef.rate.physical, &charDef.rate.magic, &charDef.rate.water, &charDef.rate.fire,
		&charDef.rate.earth, &charDef.rate.air, &charDef.rate.poison, &charDef.rate.lightning, &charDef.rate.ice, &charDef.rate.physBlock, &charDef.rate.skillBlock, &charDef.rate.heroic,
		&charDef.rate.heroicMelee, &charDef.rate.heroicRange, &charDef.rate.heroicMagic);

	SharedPtr<AlefPacket> factorDefPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DEFENSE, factorDefPointPkt, factorDefRatePkt);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	//Int32 i32AtkRng = 50, i32AtkSpd = 60;
	SharedPtr<AlefPacket> factorAttackPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTACK, &charAtk.range, &charAtk.hitRange, &charAtk.attackSpeed, &charAtk.cast, &charAtk.cooldown, 
		&charAtk.accuracy, &charAtk.meleeDodge, &charAtk.rangeDodge);

	//
	SharedPtr<AlefPacket> factorItemPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ITEM, &charItem.durability, &charItem.hand, &charItem.rank, &charItem.physRank, &charItem.maxDura, &charItem.gacha);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorDirtPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DIRT, &charDirt.duration, &charDirt.intensity, &charDirt.range, &charDirt.target, &charDirt.skillLevel, &charDirt.skillPoint, &charDirt.skillXP, &charDirt.heroicPoint);

	//Alef::AGPMCHAR_FACTOR_PRICE Skipped
	SharedPtr<AlefPacket> factorPricePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_PRICE, &charPrice.npcPrice, &charPrice.playerPrice, &charPrice.moneyHigh, &charPrice.moneyLow);

	//{	Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorResPkt = buildFactorResultPacket(charRes);

	//{	Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTORS, factorResPkt, factorStatusPkt, factorTypePkt, factorPointPkt, factorPointMaxPkt, factorRecoveryPkt, factorAttributePkt, factorDmgPkt, factorDefPkt, factorAttackPkt, factorItemPkt, factorDirtPkt, factorPricePkt);

	return factorPkt;
}

SharedPtr<AlefPacket> AlefLoginClientLogin::buildFactorResultPacket(CharacterFactorResult& result)
{
	CharacterFactorStatus charStatus = result.status;
	CharacterFactorType charType = result.type;
	CharacterFactorPoint charPoint = result.point;
	CharacterFactorPointMax charPointMax = result.pointMax;
	CharacterFactorRecovery charRecovery = result.recovery;
	CharacterFactorAttribute charAttribute = result.attribute;
	CharacterFactorDamage charDmg = result.damage;
	CharacterFactorDefense charDef = result.defense;
	CharacterFactorAttack charAtk = result.attack;
	CharacterFactorDirt charDirt = result.dirt;
	CharacterFactorPrice charPrice = result.price;
	CharacterFactorItem charItem = result.item;

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorStatusPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_STATUS,
		&charStatus.stamina, &charStatus.strength, &charStatus.intelligence, &charStatus.dexterity, &charStatus.charisma, &charStatus.luck,
		&charStatus.wisdom, &charStatus.level, &charStatus.moveSpeed, &charStatus.runSpeed, &charStatus.unionRank, &charStatus.villainPoints, &charStatus.unk);

	//{	Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorTypePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_TYPE, &charType.charRace, &charType.charGender, &charType.charClass);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32}
	SharedPtr<AlefPacket> factorPointPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINT, &charPoint.hp, &charPoint.mana, &charPoint.skillPoints, &charPoint.xpLow,
		&charPoint.xpHigh, &charPoint.AP, &charPoint.MAP, &charPoint.MI, &charPoint.agro, &charPoint.damage, &charPoint.damageMagic, &charPoint.damageWater, &charPoint.damageFire,
		&charPoint.damageEarth, &charPoint.damageAir, &charPoint.damagePoison, &charPoint.damageLightning, &charPoint.damageIce, &charPoint.bonusXP, &charPoint.damageHeroic);

	//{	Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorPointMaxPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_POINTMAX, &charPointMax.maxHP, &charPointMax.maxMana, &charPointMax.maxSkillPoints,
		&charPointMax.xpLow, &charPointMax.xpHigh, &charPointMax.AP, &charPointMax.MAP, &charPointMax.MI, &charPointMax.AR, &charPointMax.DR, &charPointMax.MAR);

	//Alef::AGPMCHAR_FACTOR_RECOVERY Skipped
	SharedPtr<AlefPacket> factorRecoveryPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_RECOVERY, &charRecovery.hpRecover, &charRecovery.manaRecovery, &charRecovery.spRecovery);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorAttributePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charAttribute.physical, &charAttribute.magic, &charAttribute.water, &charAttribute.fire,
		&charAttribute.earth, &charAttribute.air, &charAttribute.poison, &charAttribute.lightning, &charAttribute.ice, &charAttribute.physBlock, &charAttribute.skillBlock, &charAttribute.heroic,
		&charAttribute.heroicMelee, &charAttribute.heroicRange, &charAttribute.heroicMagic);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDmgMinPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDmg.min.physical, &charDmg.min.magic, &charDmg.min.water, &charDmg.min.fire,
		&charDmg.min.earth, &charDmg.min.air, &charDmg.min.poison, &charDmg.min.lightning, &charDmg.min.ice, &charDmg.min.physBlock, &charDmg.min.skillBlock, &charDmg.min.heroic,
		&charDmg.min.heroicMelee, &charDmg.min.heroicRange, &charDmg.min.heroicMagic);

	SharedPtr<AlefPacket> factorDmgMaxPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDmg.max.physical, &charDmg.max.magic, &charDmg.max.water, &charDmg.max.fire,
		&charDmg.max.earth, &charDmg.max.air, &charDmg.max.poison, &charDmg.max.lightning, &charDmg.max.ice, &charDmg.max.physBlock, &charDmg.max.skillBlock, &charDmg.max.heroic,
		&charDmg.max.heroicMelee, &charDmg.max.heroicRange, &charDmg.max.heroicMagic);

	SharedPtr<AlefPacket> factorDmgPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DMG, factorDmgMinPkt, factorDmgMaxPkt);

	//{ Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorDefPointPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDef.point.physical, &charDef.point.magic, &charDef.point.water, &charDef.point.fire,
		&charDef.point.earth, &charDef.point.air, &charDef.point.poison, &charDef.point.lightning, &charDef.point.ice, &charDef.point.physBlock, &charDef.point.skillBlock, &charDef.point.heroic,
		&charDef.point.heroicMelee, &charDef.point.heroicRange, &charDef.point.heroicMagic);

	SharedPtr<AlefPacket> factorDefRatePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTRIBUTE, &charDef.rate.physical, &charDef.rate.magic, &charDef.rate.water, &charDef.rate.fire,
		&charDef.rate.earth, &charDef.rate.air, &charDef.rate.poison, &charDef.rate.lightning, &charDef.rate.ice, &charDef.rate.physBlock, &charDef.rate.skillBlock, &charDef.rate.heroic,
		&charDef.rate.heroicMelee, &charDef.rate.heroicRange, &charDef.rate.heroicMagic);

	SharedPtr<AlefPacket> factorDefPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DEFENSE, factorDefPointPkt, factorDefRatePkt);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	//Int32 i32AtkRng = 50, i32AtkSpd = 60;
	SharedPtr<AlefPacket> factorAttackPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ATTACK, &charAtk.range, &charAtk.hitRange, &charAtk.attackSpeed, &charAtk.cast, &charAtk.cooldown,
		&charAtk.accuracy, &charAtk.meleeDodge, &charAtk.rangeDodge);

	SharedPtr<AlefPacket> factorItemPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_ITEM, &charItem.durability, &charItem.hand, &charItem.rank, &charItem.physRank, &charItem.maxDura, &charItem.gacha);

	//{ Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32, Alef::INT32 }
	SharedPtr<AlefPacket> factorDirtPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_DIRT, &charDirt.duration, &charDirt.intensity, &charDirt.range, &charDirt.target, &charDirt.skillLevel, &charDirt.skillPoint, &charDirt.skillXP, &charDirt.heroicPoint);

	//Alef::AGPMCHAR_FACTOR_PRICE Skipped
	SharedPtr<AlefPacket> factorPricePkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTOR_PRICE, &charPrice.npcPrice, &charPrice.playerPrice, &charPrice.moneyHigh, &charPrice.moneyLow);

	//{	Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET, Alef::PACKET }
	SharedPtr<AlefPacket> factorResPkt = pktInterface->buildMiniPacket(Alef::AGPMCHAR_FACTORS, 0, factorStatusPkt, factorTypePkt, factorPointPkt, factorPointMaxPkt, factorRecoveryPkt, factorAttributePkt, factorDmgPkt, factorDefPkt, factorAttackPkt, factorItemPkt, factorDirtPkt, factorPricePkt);

	return factorResPkt;
}