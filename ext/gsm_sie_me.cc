// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sie_me.cc
// *
// * Purpose: Mobile Equipment/Terminal Adapter and SMS functions
// *          (According to "AT command set for S45 Siemens mobile phones"
// *           v1.8, 26. July 2001 - Common AT prefix is "^S")
// *
// * Author:  Christian W. Zuckschwerdt  <zany@triq.net>
// *
// * Created: 2001-12-15
// *************************************************************************

#ifdef HAVE_CONFIG_H
  #include <gsm_config.h>
#endif
#include <gsmlib/gsm_nls.h>
#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_parser.h>
#include <gsmlib/gsm_util.h>
#include <gsm_sie_me.h>
#include <iostream>

using namespace gsmlib;

// SieMe members

void SieMe::init()
{
}

SieMe::SieMe(Ref<Port> port)  : MeTa::MeTa(port)
{
  // initialize Siemens ME

  init();
}

std::vector<std::string> SieMe::getSupportedPhonebooks()
{
  Parser p(_at->chat("^SPBS=?", "^SPBS:"));
  return p.parseStringList();
}

std::string SieMe::getCurrentPhonebook()
{
  if (_lastPhonebookName == "")
    {
      Parser p(_at->chat("^SPBS?", "^SPBS:"));
      // answer is e.g. ^SPBS: "SM",41,250
      _lastPhonebookName = p.parseString();
      p.parseComma();
      p.parseInt();
      p.parseComma();
      p.parseInt();
    }
  return _lastPhonebookName;
}

void SieMe::setPhonebook(std::string phonebookName)
{
  if (phonebookName != _lastPhonebookName)
    {
      _at->chat("^SPBS=\"" + phonebookName + "\"");
      _lastPhonebookName = phonebookName;
    }
}


IntRange SieMe:: getSupportedSignalTones()
{
  Parser p(_at->chat("^SPST=?", "^SPST:"));
  // ^SPST: (0-4),(0,1)
  IntRange typeRange = p.parseRange();
  p.parseComma();
  std::vector<bool> volumeList = p.parseIntList();
  return typeRange;
}

void SieMe:: playSignalTone(int tone)
{
  _at->chat("^SPST=" + intToStr(tone) + ",1");
}

void SieMe:: stopSignalTone(int tone)
{
  _at->chat("^SPST=" + intToStr(tone) + ",0");
}


IntRange SieMe::getSupportedRingingTones()  // (AT^SRTC=?)
{
  Parser p(_at->chat("^SRTC=?", "^SRTC:"));
  // ^SRTC: (0-42),(1-5)
  IntRange typeRange = p.parseRange();
  p.parseComma();
  p.parseRange();
  return typeRange;
}

int SieMe::getCurrentRingingTone()  // (AT^SRTC?)
{
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  int type = p.parseInt();
  p.parseComma();
  p.parseInt();
  p.parseComma();
  p.parseInt();
  return type;
}

void SieMe::setRingingTone(int tone, int volume)
{
  _at->chat("^SRTC=" + intToStr(tone) + "," + intToStr(volume));
}

void SieMe:: playRingingTone()
{
  // get ringing bool
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  p.parseInt();
  p.parseComma();
  p.parseInt();
  p.parseComma();
  int ringing = p.parseInt();

  if (ringing == 0)
    toggleRingingTone();
}

void SieMe::stopRingingTone()
{
  // get ringing bool
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  p.parseInt();
  p.parseComma();
  p.parseInt();
  p.parseComma();
  int ringing = p.parseInt();

  if (ringing == 1)
    toggleRingingTone();
}

void SieMe::toggleRingingTone()  // (AT^SRTC)
{
  _at->chat("^SRTC");
}

// Siemens get supported binary read
std::vector<ParameterRange> SieMe::getSupportedBinaryReads()
{
  Parser p(_at->chat("^SBNR=?", "^SBNR:"));
  // ^SBNR: ("bmp",(0-3)),("mid",(0-4)),("vcf",(0-500)),("vcs",(0-50))

  return p.parseParameterRangeList();
}

// Siemens get supported binary write
std::vector<ParameterRange> SieMe::getSupportedBinaryWrites()
{
  Parser p(_at->chat("^SBNW=?", "^SBNW:"));
  // ^SBNW: ("bmp",(0-3)),("mid",(0-4)),("vcf",(0-500)),("vcs",(0-50)),("t9d",(0))

  return p.parseParameterRangeList();
}

// Siemens Binary Read
BinaryObject SieMe::getBinary(std::string type, int subtype)
{
  // expect several response lines
  std::vector<std::string> result;
  result = _at->chatv("^SBNR=\"" + type + "\"," + intToStr(subtype), "^SBNR:");
  // "bmp",0,1,5 <CR><LF> pdu <CR><LF> "bmp",0,2,5 <CR><LF> ...
  // most likely to be PDUs of 382 chars (191 * 2)
  std::string pdu;
  int fragmentCount = 0;
  for (std::vector<std::string>::iterator i = result.begin(); i != result.end(); ++i)
    {
      ++fragmentCount;
      // parse header
      Parser p(*i);
      std::string fragmentType = p.parseString();
      if (fragmentType != type)
	throw GsmException(_("bad PDU type"), ChatError);
      p.parseComma();
      int fragmentSubtype = p.parseInt();
      if (fragmentSubtype != subtype)
	throw GsmException(_("bad PDU subtype"), ChatError);
      p.parseComma();
      int fragmentNumber = p.parseInt();
      if (fragmentNumber != fragmentCount)
	throw GsmException(_("bad PDU number"), ChatError);
      p.parseComma();
      int numberOfFragments = p.parseInt();
      if (fragmentNumber > numberOfFragments)
	throw GsmException(_("bad PDU number"), ChatError);

      // concat pdu fragment
      ++i;
      pdu += *i;
    }

  BinaryObject bnr;
  bnr._type = type;
  bnr._subtype = subtype;
  bnr._size = pdu.length() / 2;
  bnr._data = new unsigned char[pdu.length() / 2];
  if (! hexToBuf(pdu, bnr._data))
    throw GsmException(_("bad hexadecimal PDU format"), ChatError);

  return bnr;
}

// Siemens Binary Write
void SieMe::setBinary(std::string type, int subtype, BinaryObject obj)
  
{
  if (obj._size <= 0)
    throw GsmException(_("bad object"), ParameterError);

  // Limitation: The maximum pdu size is 176 bytes (or 352 characters)
  // this should be a configurable field 
  int maxPDUsize = 176;
  int numberOfPDUs = (obj._size + maxPDUsize - 1) / maxPDUsize;
  unsigned char *p = obj._data;

  for (int i = 1; i <= numberOfPDUs; ++i)
    {
      // construct pdu
      int size = maxPDUsize;
      if (i == numberOfPDUs)
	size = obj._size - (numberOfPDUs - 1) * maxPDUsize;
      std::string pdu = bufToHex(p, size);
      p += size;

      cout << "processing " << i << " of " << numberOfPDUs
	   << " of " << size << " bytes." << endl;
      cout << "^SBNW=\"" + type + "\"," + intToStr(subtype) + ","
	+ intToStr(i) + "," + intToStr(numberOfPDUs) << endl;
      cout << pdu << endl;

      _at->sendPdu("^SBNW=\"" + type + "\"," + intToStr(subtype) + ","
		   + intToStr(i) + "," + intToStr(numberOfPDUs), "",
		   pdu, true);
      cout << "OK" << endl;
    }
}
