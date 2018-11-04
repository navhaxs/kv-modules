
const char* EDDOnlineUnlockStatus::activeName   = "active";
const char* EDDOnlineUnlockStatus::enabledName  = "enabled";
const char* EDDOnlineUnlockStatus::licenseName  = "license";

namespace edd {

static const char* nodeName     = "edd";
static const char* keyName      = "key";

static String getMessageForError (const String& error)
{
    const String e = error.trim().toLowerCase();
    
    if      (e == "missing")                return "License key not found";
    else if (e == "license_not_activable")  return "License cannot be activated";
    else if (e == "no_activations_left")    return "You have reached your activation limit";
    else if (e == "expired")                return "License is expired";
    else if (e == "key_mismatch")           return "License key does not match for product";
    else if (e == "invalid_item_id")        return "Invalid product";
    else if (e == "item_name_mismatch")     return "Item name mismatch";
    else if (e == "revoked")                return "License has been revoked or disabled. If you believe this is an error please contact support.";
    
    return "Unknown server error";
}

/** Data returned from an EDD Software Licensing api call. Not all fields are applicable
    for every request.  See the EDD documentation for details
 */
struct ApiResponseData
{
    // EDD standard fields
    bool success;           ///< True if the request was successful
    String error;           ///< An error identifer if failed
    String license;         ///< Status code of result (not the license key!)
    String itemName;        ///< The product name returned
    String checksum;        ///< Generated MD5 checksum from EDD
    String expires;         ///< Either a date or "lifetime"
    int paymentID;          ///< The payment ID
    String customerName;    ///< The name of the customer
    String customerEmail;   ///< The email of the customer
    int priceID;            ///< The variable price ID for this license
    
    // Extra fields
    String key;             ///< The keyfile generated by the server
    
    ApiResponseData() : success (false), paymentID (0), priceID(0) { }
};

static ValueTree decryptValueTree (String hexData, RSAKey rsaPublicKey)
{
    BigInteger val;
    val.parseString (hexData, 16);
    
    RSAKey key (rsaPublicKey);
    jassert (key.isValid());
    
    ValueTree edd;
    
    if (! val.isZero())
    {
        key.applyToValue (val);
        const MemoryBlock mb (val.toMemoryBlock());
        if (CharPointer_UTF8::isValidString (static_cast<const char*> (mb.getData()), (int) mb.getSize()))
            if (ScopedPointer<XmlElement> xml = XmlDocument::parse (mb.toString()))
                edd = ValueTree::fromXml (*xml);
    }
    
    return edd;
}

static ApiResponseData processJSONResponse (const var& json)
{
    ApiResponseData data;
    if (auto* const object = json.getDynamicObject())
    {
        // If you hit this, then the server isn't responding correctly, check
        // your EDD settings and make sure required extensions are enabled
        // and configured.
        jassert (object->hasProperty ("success"));
    
        data.success        = (bool) object->getProperty ("success");
        data.error          = object->getProperty("error").toString().trim();
        data.license        = object->getProperty("license").toString().trim();
        data.itemName       = object->getProperty("item_name").toString().trim();
        data.checksum       = object->getProperty("checksum").toString().trim();
        data.expires        = object->getProperty("expires").toString().trim();
        data.paymentID      = (int) object->getProperty ("payment_id");
        data.customerName   = object->getProperty("customer_name").toString().trim();
        data.customerEmail  = object->getProperty("customer_email").toString().trim();
        data.priceID        = (int) object->getProperty("price_id");
        
        MemoryOutputStream mo;
        if (Base64::convertFromBase64 (mo, object->getProperty("key").toString()))
        {
            MemoryBlock mb = mo.getMemoryBlock();
            if (CharPointer_UTF8::isValidString ((const char*) mb.getData(), (int) mb.getSize()))
                data.key = String::fromUTF8 ((const char*) mb.getData(), (int) mb.getSize());
            
        }
        mo.flush();
    }
    else
    {
        data.success = false;
        data.error = "invalid_response";
    }
    
    return data;
}

}

EDDOnlineUnlockStatus::EDDOnlineUnlockStatus() : edd (edd::nodeName) { }
EDDOnlineUnlockStatus::~EDDOnlineUnlockStatus() { }

String EDDOnlineUnlockStatus::readReplyFromWebserver (const String& email, const String& password)
{
    const RSAKey publicKey (getPublicKey());
    
    StringPairArray params;
    params.set ("kv_action", "authorize_product");
    params.set ("product_id", getProductID());
    params.set ("mach", getLocalMachineIDs()[0]);
    params.set ("login", email);
    params.set ("password", password);
    
    const URL url (getServerAuthenticationURL().withParameters (params));
    DBG("[edd] authenticating @ " << url.toString (true));
    
    if (ScopedPointer<XmlElement> xml = url.readEntireXmlStream (true))
    {
        if (auto* keyElement = xml->getChildByName ("KEY"))
        {
            const String keyText (keyElement->getAllSubText().trim());
            edd = edd::decryptValueTree (keyText.fromFirstOccurrenceOf ("#", true, true),
                                         publicKey).getChildWithName (edd::nodeName);
        }
        
        return xml->createDocument (String());
    }
    
    return String();
}

void EDDOnlineUnlockStatus::eddRestoreState (const String& state)
{
    const RSAKey publicKey (getPublicKey());
    edd = ValueTree (edd::nodeName);
    
    MemoryBlock mb; mb.fromBase64Encoding (state);
    if (mb.getSize() > 0)
    {
        const ValueTree reg = ValueTree::readFromGZIPData (mb.getData(), mb.getSize());
        const String keyText = reg[edd::keyName].toString().fromFirstOccurrenceOf ("#", true, true);
        edd = edd::decryptValueTree(keyText, publicKey).getChildWithName(edd::nodeName);
    }
    
    if (! edd.isValid() || !edd.hasType (edd::nodeName))
        edd = ValueTree (edd::nodeName);
}

OnlineUnlockStatus::UnlockResult EDDOnlineUnlockStatus::activateLicense (const String& license,
                                                                         const String& email,
                                                                         const String& password)
{
    OnlineUnlockStatus::UnlockResult r;
    const URL url (getServerAuthenticationURL()
             .withParameter ("edd_action", "activate_license")
             .withParameter ("item_id", getProductID())
             .withParameter ("license", license.trim())
             .withParameter ("url", getLocalMachineIDs()[0]));
    DBG("connecting: " << url.toString (true));
    
    var response;
    Result result (JSON::parse (url.readEntireTextStream(), response));
    if (result.failed())
    {
        r.errorMessage = "Corrupt response received from server. Please try again.";
        r.succeeded = false;
        return r;
    }
    
    DBG ("JSON Response:");
    DBG (JSON::toString (response, false));

    r.succeeded = false;
    const edd::ApiResponseData data (edd::processJSONResponse (response));
    
    if (! data.success)
    {
        r.succeeded = false;
        r.errorMessage = edd::getMessageForError (data.error);
        if (data.key.isNotEmpty())
        {
            applyKeyFile (data.key);
            const ValueTree reg = edd::decryptValueTree (data.key.fromFirstOccurrenceOf ("#", true, true), getPublicKey());
            edd = reg.getChildWithName (edd::nodeName);
        }
        return r;
    }
    
    if (data.key.isNotEmpty())
        r.succeeded = applyKeyFile (data.key);
    else
        r.errorMessage = "Server did not return a key file.";
    
    if (! r.succeeded && r.errorMessage.isEmpty()) {
        r.errorMessage = "Key file is not compatible with this computer.";
        DBG("NOT COMPAT: " << data.key);
        const ValueTree reg = edd::decryptValueTree (data.key.fromFirstOccurrenceOf ("#", true, true), getPublicKey());
        DBG(newLine << "XML:");
        DBG(reg.toXmlString());
    }
    if (r.succeeded && r.informativeMessage.isEmpty())
        r.informativeMessage = "Successfully activated this machine";
    
    if (r.succeeded)
    {
        const ValueTree reg = edd::decryptValueTree (data.key.fromFirstOccurrenceOf ("#", true, true), getPublicKey());
        edd = reg.getChildWithName (edd::nodeName);
    }
    
    return r;
}

OnlineUnlockStatus::UnlockResult EDDOnlineUnlockStatus::deactivateLicense (const String& license)
{
    OnlineUnlockStatus::UnlockResult r;
    r.succeeded = false;
    const URL url (getServerAuthenticationURL()
             .withParameter ("edd_action", "deactivate_license")
             .withParameter ("item_id", getProductID())
             .withParameter ("license", license)
             .withParameter ("url", getLocalMachineIDs()[0]));
    DBG("connecting: " << url.toString (true));
    
    const String responseText = url.readEntireTextStream();
    
    var response;
    Result result (JSON::parse (responseText, response));
    if (result.failed())
    {
        r.errorMessage = "Corrupt response received from server. Please try again.";
        r.succeeded = false;
        return r;
    }
    
    DBG ("JSON Response:");
    DBG (JSON::toString (response, false));
    
    const edd::ApiResponseData data (edd::processJSONResponse (response));
    
    if (! data.success)
    {
        r.succeeded = false;
        r.errorMessage = data.error.isNotEmpty()
            ? edd::getMessageForError (data.error)
            : "Could not deactivate license. Please try again later.";
        return r;
    }
    
    if (data.key.isNotEmpty())
    {
        applyKeyFile (data.key);
        const ValueTree reg = edd::decryptValueTree (data.key.fromFirstOccurrenceOf ("#", true, true), getPublicKey());
        edd = reg.getChildWithName (edd::nodeName);
    }
    else
    {
        applyKeyFile ("#nil");
    }
    
    r.succeeded = true;
    r.errorMessage = String();
    r.informativeMessage = "License deactivation was successful.";
    return r;
}

OnlineUnlockStatus::UnlockResult EDDOnlineUnlockStatus::checkLicense (const String& license)
{
    OnlineUnlockStatus::UnlockResult r;
    const URL url (getServerAuthenticationURL()
                   .withParameter ("edd_action", "check_license")
                   .withParameter ("item_id", getProductID())
                   .withParameter ("license", license.trim())
                   .withParameter ("url", getLocalMachineIDs()[0]));
    DBG("connecting: " << url.toString (true));
    
    var response;
    Result result (JSON::parse (url.readEntireTextStream(), response));
    if (result.failed())
    {
        r.errorMessage = "Corrupt response received from server. Please try again.";
        r.succeeded = false;
        return r;
    }
    
    DBG ("JSON Response:");
    DBG (JSON::toString (response, false));
    
    const edd::ApiResponseData data (edd::processJSONResponse (response));
    if (! data.success)
    {
        r.succeeded = false;
        r.errorMessage = edd::getMessageForError (data.error);
        return r;
    }
    
    if (data.key.isNotEmpty())
    {
        applyKeyFile (data.key);
        const ValueTree reg = edd::decryptValueTree (data.key.fromFirstOccurrenceOf ("#", true, true), getPublicKey());
        edd = reg.getChildWithName (edd::nodeName);
    }
    else
    {
        applyKeyFile ("#nil");
    }

    DBG("KEYFILE: " << newLine << data.key);

    r.succeeded = true;
    r.errorMessage = r.informativeMessage = String();
    return r;
}
