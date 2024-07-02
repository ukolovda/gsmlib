// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sorted_phonebook_base.cc
// *
// * Purpose: Virtual base class for alphabetically sorted phonebook
// *          The infrastructure in this module allows custom backends for
// *          storing phonebook entries to be integrated into gsmlib
// *          (eg. LDAP- or RDBMS-based phonebook stores).
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 5.6.2000
// *************************************************************************

#ifdef HAVE_CONFIG_H
#include <gsm_config.h>
#endif
#include <gsmlib/gsm_sorted_phonebook_base.h>
#include <gsmlib/gsm_error.h>
#include <gsmlib/gsm_util.h>
#include <gsmlib/gsm_nls.h>

#include <assert.h>

using namespace gsmlib;

// PhonebookEntryBase members

void PhonebookEntryBase::set(std::string telephone, std::string text, int index,
                             bool useIndex)

{
  checkTextAndTelephone(text, telephone);

  _changed = true;
  _telephone = telephone;
  _text = text;
  _useIndex = useIndex;
  if (index != -1)
    _index = index;
}

bool PhonebookEntryBase::operator==(const PhonebookEntryBase &e) const
{
  assert(! ((_useIndex || e._useIndex) &&
            (_index == -1 || e._index == -1)));
  return _telephone == e._telephone && _text == e._text &&
    (! (_useIndex || e._useIndex) || _index == e._index);
}

std::string PhonebookEntryBase::text() const
{
  return _text;
}

std::string PhonebookEntryBase::telephone() const
{
  return _telephone;
}

bool PhonebookEntryBase::empty() const
{
  return (text() == "") && (telephone() == "");
}

Ref<PhonebookEntryBase> PhonebookEntryBase::clone()
{
  Ref<PhonebookEntryBase> result = new PhonebookEntryBase(*this);
  return result;
}

PhonebookEntryBase::PhonebookEntryBase(const PhonebookEntryBase &e)

{
  set(e._telephone, e._text, e._index, e._useIndex);
}

PhonebookEntryBase &PhonebookEntryBase::operator=(const PhonebookEntryBase &e)

{
  set(e._telephone, e._text, e._index, e._useIndex);
  return *this;
}

// CustomPhonebookRegistry members

std::map<std::string, CustomPhonebookFactory*>
*CustomPhonebookRegistry::_factoryList = NULL;

void CustomPhonebookRegistry::
registerCustomPhonebookFactory(std::string backendName,
                               CustomPhonebookFactory *factory)

{
  if (_factoryList == NULL)
    _factoryList = new std::map<std::string, CustomPhonebookFactory*>;
  backendName = lowercase(backendName);
  if (_factoryList->find(backendName) != _factoryList->end())
    throw GsmException(stringPrintf(_("backend '%s' already registered"),
                                    backendName.c_str()), ParameterError);
}
      
SortedPhonebookRef CustomPhonebookRegistry::
createPhonebook(std::string backendName, std::string source) 
{
  if (_factoryList == NULL)
    _factoryList = new std::map<std::string, CustomPhonebookFactory*>;
  backendName = lowercase(backendName);
  if (_factoryList->find(backendName) == _factoryList->end())
    throw GsmException(stringPrintf(_("backend '%s' not registered"),
                                    backendName.c_str()), ParameterError);
  return (*_factoryList)[backendName]->createPhonebook(source);
}
