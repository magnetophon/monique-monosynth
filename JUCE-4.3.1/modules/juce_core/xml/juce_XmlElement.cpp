/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 - ROLI Ltd.

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

namespace
{
    inline bool isValidXmlNameStartCharacter (const juce_wchar character) noexcept
    {
        return character == ':'
            || character == '_'
            || (character >= 'a'     && character <= 'z')
            || (character >= 'A'     && character <= 'Z')
            || (character >= 0xc0    && character <= 0xd6)
            || (character >= 0xd8    && character <= 0xf6)
            || (character >= 0xf8    && character <= 0x2ff)
            || (character >= 0x370   && character <= 0x37d)
            || (character >= 0x37f   && character <= 0x1fff)
            || (character >= 0x200c  && character <= 0x200d)
            || (character >= 0x2070  && character <= 0x218f)
            || (character >= 0x2c00  && character <= 0x2fef)
            || (character >= 0x3001  && character <= 0xd7ff)
            || (character >= 0xf900  && character <= 0xfdcf)
            || (character >= 0xfdf0  && character <= 0xfffd)
            || (character >= 0x10000 && character <= 0xeffff);
    }

    inline bool isValidXmlNameBodyCharacter (const juce_wchar character) noexcept
    {
        return isValidXmlNameStartCharacter (character)
            || character == '-'
            || character == '.'
            || character == 0xb7
            || (character >= '0'    && character <= '9')
            || (character >= 0x300  && character <= 0x036f)
            || (character >= 0x203f && character <= 0x2040);
    }
}

XmlElement::XmlAttributeNode::XmlAttributeNode (const XmlAttributeNode& other) noexcept
    : name (other.name),
      value (other.value)
{
}

XmlElement::XmlAttributeNode::XmlAttributeNode (const Identifier& n, const String& v) noexcept
    : name (n), value (v)
{
//    jassert (isValidXmlName (name));
}

XmlElement::XmlAttributeNode::XmlAttributeNode (String::CharPointerType nameStart, String::CharPointerType nameEnd)
    : name (nameStart, nameEnd)
{
    //jassert (isValidXmlName (name));
}

//==============================================================================
XmlElement::XmlElement (const String& tag)
    : tagName (StringPool::getGlobalPool().getPooledString (tag))
{
    jassert (isValidXmlName (tagName));
}

XmlElement::XmlElement (const char* tag)
    : tagName (StringPool::getGlobalPool().getPooledString (tag))
{
    jassert (isValidXmlName (tagName));
}

XmlElement::XmlElement (StringRef tag)
    : tagName (StringPool::getGlobalPool().getPooledString (tag))
{
    jassert (isValidXmlName (tagName));
}

XmlElement::XmlElement (const Identifier& tag)
    : tagName (tag.toString())
{
    jassert (isValidXmlName (tagName));
}

XmlElement::XmlElement (String::CharPointerType tagNameStart, String::CharPointerType tagNameEnd)
    : tagName (StringPool::getGlobalPool().getPooledString (tagNameStart, tagNameEnd))
{
    jassert (isValidXmlName (tagName));
}

XmlElement::XmlElement (int /*dummy*/) noexcept
{
}

XmlElement::XmlElement (const XmlElement& other)
    : tagName (other.tagName)
{
    copyChildrenAndAttributesFrom (other);
}

XmlElement& XmlElement::operator= (const XmlElement& other)
{
    if (this != &other)
    {
        removeAllAttributes();
        deleteAllChildElements();
        tagName = other.tagName;
        copyChildrenAndAttributesFrom (other);
    }

    return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
XmlElement::XmlElement (XmlElement&& other) noexcept
    : nextListItem      (static_cast<LinkedListPointer<XmlElement>&&> (other.nextListItem)),
      firstChildElement (static_cast<LinkedListPointer<XmlElement>&&> (other.firstChildElement)),
      attributes        (static_cast<LinkedListPointer<XmlAttributeNode>&&> (other.attributes)),
      tagName           (static_cast<String&&> (other.tagName))
{
}

XmlElement& XmlElement::operator= (XmlElement&& other) noexcept
{
    jassert (this != &other); // hopefully the compiler should make this situation impossible!

    removeAllAttributes();
    deleteAllChildElements();

    nextListItem      = static_cast<LinkedListPointer<XmlElement>&&> (other.nextListItem);
    firstChildElement = static_cast<LinkedListPointer<XmlElement>&&> (other.firstChildElement);
    attributes        = static_cast<LinkedListPointer<XmlAttributeNode>&&> (other.attributes);
    tagName           = static_cast<String&&> (other.tagName);

    return *this;
}
#endif

void XmlElement::copyChildrenAndAttributesFrom (const XmlElement& other)
{
    jassert (firstChildElement.get() == nullptr);
    firstChildElement.addCopyOfList (other.firstChildElement);

    jassert (attributes.get() == nullptr);
    attributes.addCopyOfList (other.attributes);
}

XmlElement::~XmlElement() noexcept
{
    firstChildElement.deleteAll();
    attributes.deleteAll();
}

//==============================================================================
namespace XmlOutputFunctions
{
   #if 0 // (These functions are just used to generate the lookup table used below)
    bool isLegalXmlCharSlow (const juce_wchar character) noexcept
    {
        if ((character >= 'a' && character <= 'z')
             || (character >= 'A' && character <= 'Z')
                || (character >= '0' && character <= '9'))
            return true;

        const char* t = " .,;:-()_+=?!'#@[]/\\*%~{}$|";

        do
        {
            if (((juce_wchar) (uint8) *t) == character)
                return true;
        }
        while (*++t != 0);

        return false;
    }

    void generateLegalCharLookupTable()
    {
        uint8 n[32] = { 0 };
        for (int i = 0; i < 256; ++i)
            if (isLegalXmlCharSlow (i))
                n[i >> 3] |= (1 << (i & 7));

        String s;
        for (int i = 0; i < 32; ++i)
            s << (int) n[i] << ", ";

        DBG (s);
    }
   #endif

    static bool isLegalXmlChar (const uint32 c) noexcept
    {
        static const unsigned char legalChars[] = { 0, 0, 0, 0, 187, 255, 255, 175, 255,
                                                    255, 255, 191, 254, 255, 255, 127 };
        return c < sizeof (legalChars) * 8
                 && (legalChars [c >> 3] & (1 << (c & 7))) != 0;
    }

    static void escapeIllegalXmlChars (OutputStream& outputStream, const String& text, const bool changeNewLines)
    {
        String::CharPointerType t (text.getCharPointer());

        for (;;)
        {
            const uint32 character = (uint32) t.getAndAdvance();

            if (character == 0)
                break;

            if (isLegalXmlChar (character))
            {
                outputStream << (char) character;
            }
            else
            {
                switch (character)
                {
                case '&':   outputStream << "&amp;"; break;
                case '"':   outputStream << "&quot;"; break;
                case '>':   outputStream << "&gt;"; break;
                case '<':   outputStream << "&lt;"; break;

                case '\n':
                case '\r':
                    if (! changeNewLines)
                    {
                        outputStream << (char) character;
                        break;
                    }
                    // Note: deliberate fall-through here!
                default:
                    outputStream << "&#" << ((int) character) << ';';
                    break;
                }
            }
        }
    }

    static void writeSpaces (OutputStream& out, const size_t numSpaces)
    {
        out.writeRepeatedByte (' ', numSpaces);
    }
}

void XmlElement::writeElementAsText (OutputStream& outputStream,
                                     const int indentationLevel,
                                     const int lineWrapLength) const
{
    using namespace XmlOutputFunctions;

    if (indentationLevel >= 0)
        writeSpaces (outputStream, (size_t) indentationLevel);

    if (! isTextElement())
    {
        outputStream.writeByte ('<');
        outputStream << tagName;

        {
            const size_t attIndent = (size_t) (indentationLevel + tagName.length() + 1);
            int lineLen = 0;

            for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
            {
                if (lineLen > lineWrapLength && indentationLevel >= 0)
                {
                    outputStream << newLine;
                    writeSpaces (outputStream, attIndent);
                    lineLen = 0;
                }

                const int64 startPos = outputStream.getPosition();
                outputStream.writeByte (' ');
                outputStream << att->name;
                outputStream.write ("=\"", 2);
                escapeIllegalXmlChars (outputStream, att->value, true);
                outputStream.writeByte ('"');
                lineLen += (int) (outputStream.getPosition() - startPos);
            }
        }

        if (firstChildElement != nullptr)
        {
            outputStream.writeByte ('>');

            bool lastWasTextNode = false;

            for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
            {
                if (child->isTextElement())
                {
                    escapeIllegalXmlChars (outputStream, child->getText(), false);
                    lastWasTextNode = true;
                }
                else
                {
                    if (indentationLevel >= 0 && ! lastWasTextNode)
                        outputStream << newLine;

                    child->writeElementAsText (outputStream,
                                               lastWasTextNode ? 0 : (indentationLevel + (indentationLevel >= 0 ? 2 : 0)), lineWrapLength);
                    lastWasTextNode = false;
                }
            }

            if (indentationLevel >= 0 && ! lastWasTextNode)
            {
                outputStream << newLine;
                writeSpaces (outputStream, (size_t) indentationLevel);
            }

            outputStream.write ("</", 2);
            outputStream << tagName;
            outputStream.writeByte ('>');
        }
        else
        {
            outputStream.write ("/>", 2);
        }
    }
    else
    {
        escapeIllegalXmlChars (outputStream, getText(), false);
    }
}

String XmlElement::createDocument (StringRef dtdToUse,
                                   const bool allOnOneLine,
                                   const bool includeXmlHeader,
                                   StringRef encodingType,
                                   const int lineWrapLength) const
{
    MemoryOutputStream mem (2048);
    writeToStream (mem, dtdToUse, allOnOneLine, includeXmlHeader, encodingType, lineWrapLength);

    return mem.toUTF8();
}

void XmlElement::writeToStream (OutputStream& output,
                                StringRef dtdToUse,
                                const bool allOnOneLine,
                                const bool includeXmlHeader,
                                StringRef encodingType,
                                const int lineWrapLength) const
{
    using namespace XmlOutputFunctions;

    if (includeXmlHeader)
    {
        output << "<?xml version=\"1.0\" encoding=\"" << encodingType << "\"?>";

        if (allOnOneLine)
            output.writeByte (' ');
        else
            output << newLine << newLine;
    }

    if (dtdToUse.isNotEmpty())
    {
        output << dtdToUse;

        if (allOnOneLine)
            output.writeByte (' ');
        else
            output << newLine;
    }

    writeElementAsText (output, allOnOneLine ? -1 : 0, lineWrapLength);

    if (! allOnOneLine)
        output << newLine;
}

bool XmlElement::writeToFile (const File& file,
                              StringRef dtdToUse,
                              StringRef encodingType,
                              const int lineWrapLength) const
{
    TemporaryFile tempFile (file);

    {
        FileOutputStream out (tempFile.getFile());

        if (! out.openedOk())
            return false;

        writeToStream (out, dtdToUse, false, true, encodingType, lineWrapLength);

        out.flush(); // (called explicitly to force an fsync on posix)

        if (out.getStatus().failed())
            return false;
    }

    return tempFile.overwriteTargetFileWithTemporary();
}

//==============================================================================
bool XmlElement::hasTagName (StringRef possibleTagName) const noexcept
{
    const bool matches = tagName.equalsIgnoreCase (possibleTagName);

    // XML tags should be case-sensitive, so although this method allows a
    // case-insensitive match to pass, you should try to avoid this.
    jassert ((! matches) || tagName == possibleTagName);

    return matches;
}

String XmlElement::getNamespace() const
{
    return tagName.upToFirstOccurrenceOf (":", false, false);
}

String XmlElement::getTagNameWithoutNamespace() const
{
    return tagName.fromLastOccurrenceOf (":", false, false);
}

bool XmlElement::hasTagNameIgnoringNamespace (StringRef possibleTagName) const
{
    return hasTagName (possibleTagName) || getTagNameWithoutNamespace() == possibleTagName;
}

XmlElement* XmlElement::getNextElementWithTagName (StringRef requiredTagName) const
{
    XmlElement* e = nextListItem;

    while (e != nullptr && ! e->hasTagName (requiredTagName))
        e = e->nextListItem;

    return e;
}

//==============================================================================
int XmlElement::getNumAttributes() const noexcept
{
    return attributes.size();
}

static const String& getEmptyStringRef() noexcept
{
   #if JUCE_ALLOW_STATIC_NULL_VARIABLES
    return String::empty;
   #else
    static String empty;
    return empty;
   #endif
}

const String& XmlElement::getAttributeName (const int index) const noexcept
{
    if (const XmlAttributeNode* const att = attributes [index])
        return att->name.toString();

    return getEmptyStringRef();
}

const String& XmlElement::getAttributeValue (const int index) const noexcept
{
    if (const XmlAttributeNode* const att = attributes [index])
        return att->value;

    return getEmptyStringRef();
}

XmlElement::XmlAttributeNode* XmlElement::getAttribute (StringRef attributeName) const noexcept
{
    for (XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
        if (att->name == attributeName)
            return att;

    return nullptr;
}

bool XmlElement::hasAttribute (StringRef attributeName) const noexcept
{
    return getAttribute (attributeName) != nullptr;
}

//==============================================================================
const String& XmlElement::getStringAttribute (StringRef attributeName) const noexcept
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
        return att->value;

    return getEmptyStringRef();
}

String XmlElement::getStringAttribute (StringRef attributeName, const String& defaultReturnValue) const
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
        return att->value;

    return defaultReturnValue;
}

int XmlElement::getIntAttribute (StringRef attributeName, const int defaultReturnValue) const
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
        return att->value.getIntValue();

    return defaultReturnValue;
}

double XmlElement::getDoubleAttribute (StringRef attributeName, const double defaultReturnValue) const
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
        return att->value.getDoubleValue();

    return defaultReturnValue;
}

bool XmlElement::getBoolAttribute (StringRef attributeName, const bool defaultReturnValue) const
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
    {
        const juce_wchar firstChar = *(att->value.getCharPointer().findEndOfWhitespace());

        return firstChar == '1'
            || firstChar == 't'
            || firstChar == 'y'
            || firstChar == 'T'
            || firstChar == 'Y';
    }

    return defaultReturnValue;
}

bool XmlElement::compareAttribute (StringRef attributeName,
                                   StringRef stringToCompareAgainst,
                                   const bool ignoreCase) const noexcept
{
    if (const XmlAttributeNode* att = getAttribute (attributeName))
        return ignoreCase ? att->value.equalsIgnoreCase (stringToCompareAgainst)
                          : att->value == stringToCompareAgainst;

    return false;
}

//==============================================================================
void XmlElement::setAttribute (const Identifier& attributeName, const String& value)
{
    if (attributes == nullptr)
    {
        attributes = new XmlAttributeNode (attributeName, value);
    }
    else
    {
        for (XmlAttributeNode* att = attributes; ; att = att->nextListItem)
        {
            if (att->name == attributeName)
            {
                att->value = value;
                break;
            }

            if (att->nextListItem == nullptr)
            {
                att->nextListItem = new XmlAttributeNode (attributeName, value);
                break;
            }
        }
    }
}

void XmlElement::setAttribute (const Identifier& attributeName, const int number)
{
    setAttribute (attributeName, String (number));
}

void XmlElement::setAttribute (const Identifier& attributeName, const double number)
{
    setAttribute (attributeName, String (number, 20));
}

void XmlElement::removeAttribute (const Identifier& attributeName) noexcept
{
    for (LinkedListPointer<XmlAttributeNode>* att = &attributes;
         att->get() != nullptr;
         att = &(att->get()->nextListItem))
    {
        if (att->get()->name == attributeName)
        {
            delete att->removeNext();
            break;
        }
    }
}

void XmlElement::removeAllAttributes() noexcept
{
    attributes.deleteAll();
}

//==============================================================================
int XmlElement::getNumChildElements() const noexcept
{
    return firstChildElement.size();
}

XmlElement* XmlElement::getChildElement (const int index) const noexcept
{
    return firstChildElement [index].get();
}

XmlElement* XmlElement::getChildByName (StringRef childName) const noexcept
{
    jassert (! childName.isEmpty());

    for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
        if (child->hasTagName (childName))
            return child;

    return nullptr;
}

XmlElement* XmlElement::getChildByAttribute (StringRef attributeName, StringRef attributeValue) const noexcept
{
    jassert (! attributeName.isEmpty());

    for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
        if (child->compareAttribute (attributeName, attributeValue))
            return child;

    return nullptr;
}

void XmlElement::addChildElement (XmlElement* const newNode) noexcept
{
    if (newNode != nullptr)
    {
        // The element being added must not be a child of another node!
        jassert (newNode->nextListItem == nullptr);

        firstChildElement.append (newNode);
    }
}

void XmlElement::insertChildElement (XmlElement* const newNode, int indexToInsertAt) noexcept
{
    if (newNode != nullptr)
    {
        // The element being added must not be a child of another node!
        jassert (newNode->nextListItem == nullptr);

        firstChildElement.insertAtIndex (indexToInsertAt, newNode);
    }
}

void XmlElement::prependChildElement (XmlElement* newNode) noexcept
{
    if (newNode != nullptr)
    {
        // The element being added must not be a child of another node!
        jassert (newNode->nextListItem == nullptr);

        firstChildElement.insertNext (newNode);
    }
}

XmlElement* XmlElement::createNewChildElement (StringRef childTagName)
{
    XmlElement* const newElement = new XmlElement (childTagName);
    addChildElement (newElement);
    return newElement;
}

bool XmlElement::replaceChildElement (XmlElement* const currentChildElement,
                                      XmlElement* const newNode) noexcept
{
    if (newNode != nullptr)
    {
        if (LinkedListPointer<XmlElement>* const p = firstChildElement.findPointerTo (currentChildElement))
        {
            if (currentChildElement != newNode)
                delete p->replaceNext (newNode);

            return true;
        }
    }

    return false;
}

void XmlElement::removeChildElement (XmlElement* const childToRemove,
                                     const bool shouldDeleteTheChild) noexcept
{
    if (childToRemove != nullptr)
    {
        firstChildElement.remove (childToRemove);

        if (shouldDeleteTheChild)
            delete childToRemove;
    }
}

bool XmlElement::isEquivalentTo (const XmlElement* const other,
                                 const bool ignoreOrderOfAttributes) const noexcept
{
    if (this != other)
    {
        if (other == nullptr || tagName != other->tagName)
            return false;

        if (ignoreOrderOfAttributes)
        {
            int totalAtts = 0;

            for (const XmlAttributeNode* att = attributes; att != nullptr; att = att->nextListItem)
            {
                if (! other->compareAttribute (att->name, att->value))
                    return false;

                ++totalAtts;
            }

            if (totalAtts != other->getNumAttributes())
                return false;
        }
        else
        {
            const XmlAttributeNode* thisAtt = attributes;
            const XmlAttributeNode* otherAtt = other->attributes;

            for (;;)
            {
                if (thisAtt == nullptr || otherAtt == nullptr)
                {
                    if (thisAtt == otherAtt) // both nullptr, so it's a match
                        break;

                    return false;
                }

                if (thisAtt->name != otherAtt->name
                     || thisAtt->value != otherAtt->value)
                {
                    return false;
                }

                thisAtt = thisAtt->nextListItem;
                otherAtt = otherAtt->nextListItem;
            }
        }

        const XmlElement* thisChild = firstChildElement;
        const XmlElement* otherChild = other->firstChildElement;

        for (;;)
        {
            if (thisChild == nullptr || otherChild == nullptr)
            {
                if (thisChild == otherChild) // both 0, so it's a match
                    break;

                return false;
            }

            if (! thisChild->isEquivalentTo (otherChild, ignoreOrderOfAttributes))
                return false;

            thisChild = thisChild->nextListItem;
            otherChild = otherChild->nextListItem;
        }
    }

    return true;
}

void XmlElement::deleteAllChildElements() noexcept
{
    firstChildElement.deleteAll();
}

void XmlElement::deleteAllChildElementsWithTagName (StringRef name) noexcept
{
    for (XmlElement* child = firstChildElement; child != nullptr;)
    {
        XmlElement* const nextChild = child->nextListItem;

        if (child->hasTagName (name))
            removeChildElement (child, true);

        child = nextChild;
    }
}

bool XmlElement::containsChildElement (const XmlElement* const possibleChild) const noexcept
{
    return firstChildElement.contains (possibleChild);
}

XmlElement* XmlElement::findParentElementOf (const XmlElement* const elementToLookFor) noexcept
{
    if (this == elementToLookFor || elementToLookFor == nullptr)
        return nullptr;

    for (XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
    {
        if (elementToLookFor == child)
            return this;

        if (XmlElement* const found = child->findParentElementOf (elementToLookFor))
            return found;
    }

    return nullptr;
}

void XmlElement::getChildElementsAsArray (XmlElement** elems) const noexcept
{
    firstChildElement.copyToArray (elems);
}

void XmlElement::reorderChildElements (XmlElement** const elems, const int num) noexcept
{
    XmlElement* e = firstChildElement = elems[0];

    for (int i = 1; i < num; ++i)
    {
        e->nextListItem = elems[i];
        e = e->nextListItem;
    }

    e->nextListItem = nullptr;
}

//==============================================================================
bool XmlElement::isTextElement() const noexcept
{
    return tagName.isEmpty();
}

static const String juce_xmltextContentAttributeName ("text");

const String& XmlElement::getText() const noexcept
{
    jassert (isTextElement());  // you're trying to get the text from an element that
                                // isn't actually a text element.. If this contains text sub-nodes, you
                                // probably want to use getAllSubText instead.

    return getStringAttribute (juce_xmltextContentAttributeName);
}

void XmlElement::setText (const String& newText)
{
    if (isTextElement())
        setAttribute (juce_xmltextContentAttributeName, newText);
    else
        jassertfalse; // you can only change the text in a text element, not a normal one.
}

String XmlElement::getAllSubText() const
{
    if (isTextElement())
        return getText();

    if (getNumChildElements() == 1)
        return firstChildElement.get()->getAllSubText();

    MemoryOutputStream mem (1024);

    for (const XmlElement* child = firstChildElement; child != nullptr; child = child->nextListItem)
        mem << child->getAllSubText();

    return mem.toUTF8();
}

String XmlElement::getChildElementAllSubText (StringRef childTagName, const String& defaultReturnValue) const
{
    if (const XmlElement* const child = getChildByName (childTagName))
        return child->getAllSubText();

    return defaultReturnValue;
}

XmlElement* XmlElement::createTextElement (const String& text)
{
    XmlElement* const e = new XmlElement ((int) 0);
    e->setAttribute (juce_xmltextContentAttributeName, text);
    return e;
}

bool XmlElement::isValidXmlName (StringRef text) noexcept
{
    if (text.isEmpty() || ! isValidXmlNameStartCharacter (text.text.getAndAdvance()))
        return false;

    for (;;)
    {
        if (text.isEmpty())
            return true;

        if (! isValidXmlNameBodyCharacter (text.text.getAndAdvance()))
            return false;
    }
}

void XmlElement::addTextElement (const String& text)
{
    addChildElement (createTextElement (text));
}

void XmlElement::deleteAllTextElements() noexcept
{
    for (XmlElement* child = firstChildElement; child != nullptr;)
    {
        XmlElement* const next = child->nextListItem;

        if (child->isTextElement())
            removeChildElement (child, true);

        child = next;
    }
}
