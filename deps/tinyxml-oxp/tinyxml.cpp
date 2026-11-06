// The TinyXML interface on OXP: the tree, the printer, and a reader that
// builds the tree from OXP's events. See tinyxml.h.
// Copyright (c) 2026 AmiCradle Team
#include "tinyxml.h"

#include <stdlib.h>
#include <string.h>
#include <istream>
#include <iterator>
#include <ostream>
#include <sstream>

#include "oxp.h"

bool TiXmlBase::condenseWhiteSpace = true;

void TiXmlBase::EncodeString(const std::string& str, std::string* out)
{
    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned char c = (unsigned char)str[i];
        // a character reference already written out stays as it is
        if (c == '&' && i + 2 < str.size() && str[i + 1] == '#' && str[i + 2] == 'x')
        {
            size_t j = i + 3;
            while (j < str.size() && isxdigit((unsigned char)str[j]))
                ++j;
            if (j < str.size() && str[j] == ';' && j > i + 3)
            {
                out->append(str, i, j - i + 1);
                i = j;
                continue;
            }
        }
        switch (c)
        {
            case '&':  *out += "&amp;";  break;
            case '<':  *out += "&lt;";   break;
            case '>':  *out += "&gt;";   break;
            case '"':  *out += "&quot;"; break;
            case '\'': *out += "&apos;"; break;
            default:
                if (c < 32)
                {
                    char buf[8];
                    snprintf(buf, sizeof buf, "&#x%02X;", (unsigned)c);
                    *out += buf;
                }
                else
                    *out += (char)c;
        }
    }
}

// ---- nodes ----

TiXmlNode::TiXmlNode(NodeType _type)
    : parent(0), type(_type), firstChild(0), lastChild(0), prev(0), next(0)
{
}

TiXmlNode::~TiXmlNode()
{
    Clear();
}

void TiXmlNode::CopyTo(TiXmlNode* target) const
{
    target->SetValue(value);
    target->userData = userData;
    target->location = location;
}

void TiXmlNode::Clear()
{
    TiXmlNode* n = firstChild;
    while (n)
    {
        TiXmlNode* t = n->next;
        delete n;
        n = t;
    }
    firstChild = lastChild = 0;
}

TiXmlNode* TiXmlNode::LinkEndChild(TiXmlNode* node)
{
    if (!node)
        return 0;
    if (node->Type() == TINYXML_DOCUMENT)
    {
        delete node;
        TiXmlDocument* d = GetDocument();
        if (d)
            d->SetError(TIXML_ERROR_DOCUMENT_TOP_ONLY, "A document can only be at the top.", 0, 0);
        return 0;
    }
    node->parent = this;
    node->prev = lastChild;
    node->next = 0;
    if (lastChild)
        lastChild->next = node;
    else
        firstChild = node;
    lastChild = node;
    return node;
}

TiXmlNode* TiXmlNode::InsertEndChild(const TiXmlNode& addThis)
{
    if (addThis.Type() == TINYXML_DOCUMENT)
        return 0;
    TiXmlNode* node = addThis.Clone();
    return node ? LinkEndChild(node) : 0;
}

TiXmlNode* TiXmlNode::InsertBeforeChild(TiXmlNode* beforeThis, const TiXmlNode& addThis)
{
    if (!beforeThis || beforeThis->parent != this || addThis.Type() == TINYXML_DOCUMENT)
        return 0;
    TiXmlNode* node = addThis.Clone();
    if (!node)
        return 0;
    node->parent = this;
    node->next = beforeThis;
    node->prev = beforeThis->prev;
    if (beforeThis->prev)
        beforeThis->prev->next = node;
    else
        firstChild = node;
    beforeThis->prev = node;
    return node;
}

TiXmlNode* TiXmlNode::InsertAfterChild(TiXmlNode* afterThis, const TiXmlNode& addThis)
{
    if (!afterThis || afterThis->parent != this || addThis.Type() == TINYXML_DOCUMENT)
        return 0;
    TiXmlNode* node = addThis.Clone();
    if (!node)
        return 0;
    node->parent = this;
    node->prev = afterThis;
    node->next = afterThis->next;
    if (afterThis->next)
        afterThis->next->prev = node;
    else
        lastChild = node;
    afterThis->next = node;
    return node;
}

TiXmlNode* TiXmlNode::ReplaceChild(TiXmlNode* replaceThis, const TiXmlNode& withThis)
{
    if (!replaceThis || replaceThis->parent != this || withThis.ToDocument())
        return 0;
    TiXmlNode* node = withThis.Clone();
    if (!node)
        return 0;
    node->next = replaceThis->next;
    node->prev = replaceThis->prev;
    if (replaceThis->next)
        replaceThis->next->prev = node;
    else
        lastChild = node;
    if (replaceThis->prev)
        replaceThis->prev->next = node;
    else
        firstChild = node;
    node->parent = this;
    delete replaceThis;
    return node;
}

bool TiXmlNode::RemoveChild(TiXmlNode* removeThis)
{
    if (!removeThis || removeThis->parent != this)
        return false;
    if (removeThis->next)
        removeThis->next->prev = removeThis->prev;
    else
        lastChild = removeThis->prev;
    if (removeThis->prev)
        removeThis->prev->next = removeThis->next;
    else
        firstChild = removeThis->next;
    delete removeThis;
    return true;
}

const TiXmlNode* TiXmlNode::FirstChild(const char* _value) const
{
    for (const TiXmlNode* n = firstChild; n; n = n->next)
        if (strcmp(n->Value(), _value) == 0)
            return n;
    return 0;
}

const TiXmlNode* TiXmlNode::LastChild(const char* _value) const
{
    for (const TiXmlNode* n = lastChild; n; n = n->prev)
        if (strcmp(n->Value(), _value) == 0)
            return n;
    return 0;
}

const TiXmlNode* TiXmlNode::IterateChildren(const TiXmlNode* previous) const
{
    return previous ? previous->NextSibling() : FirstChild();
}

const TiXmlNode* TiXmlNode::IterateChildren(const char* val, const TiXmlNode* previous) const
{
    return previous ? previous->NextSibling(val) : FirstChild(val);
}

const TiXmlNode* TiXmlNode::NextSibling(const char* _value) const
{
    for (const TiXmlNode* n = next; n; n = n->next)
        if (strcmp(n->Value(), _value) == 0)
            return n;
    return 0;
}

const TiXmlNode* TiXmlNode::PreviousSibling(const char* _value) const
{
    for (const TiXmlNode* n = prev; n; n = n->prev)
        if (strcmp(n->Value(), _value) == 0)
            return n;
    return 0;
}

const TiXmlElement* TiXmlNode::FirstChildElement() const
{
    for (const TiXmlNode* n = firstChild; n; n = n->next)
        if (n->ToElement())
            return n->ToElement();
    return 0;
}

const TiXmlElement* TiXmlNode::FirstChildElement(const char* _value) const
{
    for (const TiXmlNode* n = FirstChild(_value); n; n = n->NextSibling(_value))
        if (n->ToElement())
            return n->ToElement();
    return 0;
}

const TiXmlElement* TiXmlNode::NextSiblingElement() const
{
    for (const TiXmlNode* n = next; n; n = n->next)
        if (n->ToElement())
            return n->ToElement();
    return 0;
}

const TiXmlElement* TiXmlNode::NextSiblingElement(const char* _value) const
{
    for (const TiXmlNode* n = NextSibling(_value); n; n = n->NextSibling(_value))
        if (n->ToElement())
            return n->ToElement();
    return 0;
}

const TiXmlDocument* TiXmlNode::GetDocument() const
{
    for (const TiXmlNode* n = this; n; n = n->parent)
        if (n->ToDocument())
            return n->ToDocument();
    return 0;
}

// ---- attributes ----

int TiXmlAttribute::QueryIntValue(int* ival) const
{
    char* end;
    const char* s = value.c_str();
    long v = strtol(s, &end, 10);
    if (end == s)
        return TIXML_WRONG_TYPE;
    *ival = (int)v;
    return TIXML_SUCCESS;
}

int TiXmlAttribute::QueryDoubleValue(double* dval) const
{
    char* end;
    const char* s = value.c_str();
    double v = strtod(s, &end);
    if (end == s)
        return TIXML_WRONG_TYPE;
    *dval = v;
    return TIXML_SUCCESS;
}

int TiXmlAttribute::IntValue() const
{
    return atoi(value.c_str());
}

double TiXmlAttribute::DoubleValue() const
{
    return atof(value.c_str());
}

void TiXmlAttribute::SetIntValue(int _value)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", _value);
    value = buf;
}

void TiXmlAttribute::SetDoubleValue(double _value)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%g", _value);
    value = buf;
}

void TiXmlAttribute::Print(FILE* cfile, int, std::string* str) const
{
    std::string n, v;
    EncodeString(name, &n);
    EncodeString(value, &v);
    // the value is quoted with " unless it holds one
    std::string out = (value.find('"') == std::string::npos) ? n + "=\"" + v + "\"" : n + "='" + v + "'";
    if (cfile)
        fputs(out.c_str(), cfile);
    if (str)
        *str += out;
}

// ---- elements ----

TiXmlElement::TiXmlElement(const char* _value)
    : TiXmlNode(TiXmlNode::TINYXML_ELEMENT), firstAttr(0), lastAttr(0)
{
    SetValue(_value);
}

TiXmlElement::TiXmlElement(const std::string& _value)
    : TiXmlNode(TiXmlNode::TINYXML_ELEMENT), firstAttr(0), lastAttr(0)
{
    value = _value;
}

TiXmlElement::TiXmlElement(const TiXmlElement& copy)
    : TiXmlNode(TiXmlNode::TINYXML_ELEMENT), firstAttr(0), lastAttr(0)
{
    copy.CopyTo(this);
}

TiXmlElement& TiXmlElement::operator=(const TiXmlElement& base)
{
    if (&base != this)
    {
        ClearThis();
        base.CopyTo(this);
    }
    return *this;
}

TiXmlElement::~TiXmlElement()
{
    ClearThis();
}

void TiXmlElement::ClearThis()
{
    Clear();
    TiXmlAttribute* a = firstAttr;
    while (a)
    {
        TiXmlAttribute* t = a->next;
        delete a;
        a = t;
    }
    firstAttr = lastAttr = 0;
}

TiXmlAttribute* TiXmlElement::Find(const char* name) const
{
    for (TiXmlAttribute* a = firstAttr; a; a = a->next)
        if (a->name == name)
            return a;
    return 0;
}

void TiXmlElement::AppendAttribute(const char* name, const char* val)
{
    TiXmlAttribute* a = new TiXmlAttribute(name, val);
    a->prev = lastAttr;
    if (lastAttr)
        lastAttr->next = a;
    else
        firstAttr = a;
    lastAttr = a;
}

const char* TiXmlElement::Attribute(const char* name) const
{
    const TiXmlAttribute* a = Find(name);
    return a ? a->Value() : 0;
}

const std::string* TiXmlElement::Attribute(const std::string& name) const
{
    const TiXmlAttribute* a = Find(name.c_str());
    return a ? &a->ValueStr() : 0;
}

const char* TiXmlElement::Attribute(const char* name, int* i) const
{
    const TiXmlAttribute* a = Find(name);
    if (!a)
        return 0;
    if (i)
        a->QueryIntValue(i);
    return a->Value();
}

const std::string* TiXmlElement::Attribute(const std::string& name, int* i) const
{
    const TiXmlAttribute* a = Find(name.c_str());
    if (!a)
        return 0;
    if (i)
        a->QueryIntValue(i);
    return &a->ValueStr();
}

const char* TiXmlElement::Attribute(const char* name, double* d) const
{
    const TiXmlAttribute* a = Find(name);
    if (!a)
        return 0;
    if (d)
        a->QueryDoubleValue(d);
    return a->Value();
}

const std::string* TiXmlElement::Attribute(const std::string& name, double* d) const
{
    const TiXmlAttribute* a = Find(name.c_str());
    if (!a)
        return 0;
    if (d)
        a->QueryDoubleValue(d);
    return &a->ValueStr();
}

int TiXmlElement::QueryIntAttribute(const char* name, int* ival) const
{
    const TiXmlAttribute* a = Find(name);
    return a ? a->QueryIntValue(ival) : TIXML_NO_ATTRIBUTE;
}

int TiXmlElement::QueryUnsignedAttribute(const char* name, unsigned* uval) const
{
    int i = 0;
    int r = QueryIntAttribute(name, &i);
    if (r == TIXML_SUCCESS)
        *uval = (unsigned)i;
    return r;
}

int TiXmlElement::QueryBoolAttribute(const char* name, bool* bval) const
{
    const TiXmlAttribute* a = Find(name);
    if (!a)
        return TIXML_NO_ATTRIBUTE;
    const std::string& v = a->ValueStr();
    if (v == "true" || v == "TRUE" || v == "True" || v == "yes" || v == "Yes" || v == "YES" || v == "1")
        *bval = true;
    else if (v == "false" || v == "FALSE" || v == "False" || v == "no" || v == "No" || v == "NO" || v == "0")
        *bval = false;
    else
        return TIXML_WRONG_TYPE;
    return TIXML_SUCCESS;
}

int TiXmlElement::QueryDoubleAttribute(const char* name, double* dval) const
{
    const TiXmlAttribute* a = Find(name);
    return a ? a->QueryDoubleValue(dval) : TIXML_NO_ATTRIBUTE;
}

void TiXmlElement::SetAttribute(const char* name, const char* cvalue)
{
    if (!name)
        return;
    TiXmlAttribute* a = Find(name);
    if (a)
        a->SetValue(cvalue);
    else
        AppendAttribute(name, cvalue);
}

void TiXmlElement::SetAttribute(const char* name, int val)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", val);
    SetAttribute(name, buf);
}

void TiXmlElement::SetDoubleAttribute(const char* name, double val)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%g", val);
    SetAttribute(name, buf);
}

void TiXmlElement::RemoveAttribute(const char* name)
{
    TiXmlAttribute* a = Find(name);
    if (!a)
        return;
    if (a->prev)
        a->prev->next = a->next;
    else
        firstAttr = a->next;
    if (a->next)
        a->next->prev = a->prev;
    else
        lastAttr = a->prev;
    delete a;
}

const char* TiXmlElement::GetText() const
{
    const TiXmlNode* child = FirstChild();
    if (child && child->ToText())
        return child->Value();
    return 0;
}

void TiXmlElement::CopyTo(TiXmlElement* target) const
{
    TiXmlNode::CopyTo(target);
    for (const TiXmlAttribute* a = firstAttr; a; a = a->next)
        target->SetAttribute(a->Name(), a->Value());
    for (const TiXmlNode* n = firstChild; n; n = n->NextSibling())
        target->LinkEndChild(n->Clone());
}

TiXmlNode* TiXmlElement::Clone() const
{
    TiXmlElement* clone = new TiXmlElement(Value());
    CopyTo(clone);
    return clone;
}

bool TiXmlElement::Accept(TiXmlVisitor* visitor) const
{
    if (visitor->VisitEnter(*this, firstAttr))
    {
        for (const TiXmlNode* n = FirstChild(); n; n = n->NextSibling())
            if (!n->Accept(visitor))
                break;
    }
    return visitor->VisitExit(*this);
}

// ---- comments, text, declarations, the rest ----

TiXmlNode* TiXmlComment::Clone() const
{
    TiXmlComment* clone = new TiXmlComment();
    CopyTo(clone);
    return clone;
}

bool TiXmlComment::Accept(TiXmlVisitor* visitor) const
{
    return visitor->Visit(*this);
}

bool TiXmlText::Blank() const
{
    for (size_t i = 0; i < value.size(); ++i)
        if (!isspace((unsigned char)value[i]))
            return false;
    return true;
}

void TiXmlText::CopyTo(TiXmlText* target) const
{
    TiXmlNode::CopyTo(target);
    target->cdata = cdata;
}

TiXmlNode* TiXmlText::Clone() const
{
    TiXmlText* clone = new TiXmlText("");
    CopyTo(clone);
    return clone;
}

bool TiXmlText::Accept(TiXmlVisitor* visitor) const
{
    return visitor->Visit(*this);
}

void TiXmlDeclaration::CopyTo(TiXmlDeclaration* target) const
{
    TiXmlNode::CopyTo(target);
    target->version = version;
    target->encoding = encoding;
    target->standalone = standalone;
}

TiXmlNode* TiXmlDeclaration::Clone() const
{
    TiXmlDeclaration* clone = new TiXmlDeclaration();
    CopyTo(clone);
    return clone;
}

bool TiXmlDeclaration::Accept(TiXmlVisitor* visitor) const
{
    return visitor->Visit(*this);
}

void TiXmlDeclaration::Print(FILE* cfile, int, std::string* str) const
{
    std::string out = "<?xml ";
    if (!version.empty())
        out += "version=\"" + version + "\" ";
    if (!encoding.empty())
        out += "encoding=\"" + encoding + "\" ";
    if (!standalone.empty())
        out += "standalone=\"" + standalone + "\" ";
    out += "?>";
    if (cfile)
        fputs(out.c_str(), cfile);
    if (str)
        *str += out;
}

TiXmlNode* TiXmlUnknown::Clone() const
{
    TiXmlUnknown* clone = new TiXmlUnknown();
    CopyTo(clone);
    return clone;
}

bool TiXmlUnknown::Accept(TiXmlVisitor* visitor) const
{
    return visitor->Visit(*this);
}

// Print(FILE*) of each kind: the printer's text
static void PrintNode(const TiXmlNode& node, FILE* cfile)
{
    TiXmlPrinter printer;
    node.Accept(&printer);
    fputs(printer.CStr(), cfile);
}

void TiXmlElement::Print(FILE* cfile, int) const { PrintNode(*this, cfile); }
void TiXmlComment::Print(FILE* cfile, int) const { PrintNode(*this, cfile); }
void TiXmlText::Print(FILE* cfile, int) const { PrintNode(*this, cfile); }
void TiXmlUnknown::Print(FILE* cfile, int) const { PrintNode(*this, cfile); }
void TiXmlDocument::Print(FILE* cfile, int) const { PrintNode(*this, cfile); }

// ---- the printer ----

bool TiXmlPrinter::VisitEnter(const TiXmlDocument&)
{
    return true;
}

bool TiXmlPrinter::VisitExit(const TiXmlDocument&)
{
    return true;
}

bool TiXmlPrinter::VisitEnter(const TiXmlElement& element, const TiXmlAttribute* firstAttribute)
{
    DoIndent();
    buffer += "<";
    buffer += element.Value();
    for (const TiXmlAttribute* a = firstAttribute; a; a = a->Next())
    {
        buffer += " ";
        a->Print(0, 0, &buffer);
    }
    if (!element.FirstChild())
    {
        buffer += " />";
        DoLineBreak();
    }
    else
    {
        buffer += ">";
        const TiXmlText* t = element.FirstChild()->ToText();
        // an element holding only text is written on one line
        if (t && element.LastChild() == element.FirstChild() && !t->CDATA())
            simpleTextPrint = true;
        else
            DoLineBreak();
    }
    ++depth;
    return true;
}

bool TiXmlPrinter::VisitExit(const TiXmlElement& element)
{
    --depth;
    if (element.FirstChild())
    {
        if (simpleTextPrint)
            simpleTextPrint = false;
        else
            DoIndent();
        buffer += "</";
        buffer += element.Value();
        buffer += ">";
        DoLineBreak();
    }
    return true;
}

bool TiXmlPrinter::Visit(const TiXmlText& text)
{
    if (text.CDATA())
    {
        DoIndent();
        buffer += "<![CDATA[";
        buffer += text.Value();
        buffer += "]]>";
        DoLineBreak();
    }
    else if (simpleTextPrint)
    {
        TiXmlBase::EncodeString(text.ValueStr(), &buffer);
    }
    else
    {
        DoIndent();
        TiXmlBase::EncodeString(text.ValueStr(), &buffer);
        DoLineBreak();
    }
    return true;
}

bool TiXmlPrinter::Visit(const TiXmlDeclaration& declaration)
{
    DoIndent();
    declaration.Print(0, 0, &buffer);
    DoLineBreak();
    return true;
}

bool TiXmlPrinter::Visit(const TiXmlComment& comment)
{
    DoIndent();
    buffer += "<!--";
    buffer += comment.Value();
    buffer += "-->";
    DoLineBreak();
    return true;
}

bool TiXmlPrinter::Visit(const TiXmlUnknown& unknown)
{
    DoIndent();
    buffer += "<";
    buffer += unknown.Value();
    buffer += ">";
    DoLineBreak();
    return true;
}

// ---- the handle ----

TiXmlHandle TiXmlHandle::FirstChild() const
{
    return TiXmlHandle(node ? node->FirstChild() : 0);
}

TiXmlHandle TiXmlHandle::FirstChild(const char* value) const
{
    return TiXmlHandle(node ? node->FirstChild(value) : 0);
}

TiXmlHandle TiXmlHandle::FirstChildElement() const
{
    return TiXmlHandle(node ? node->FirstChildElement() : 0);
}

TiXmlHandle TiXmlHandle::FirstChildElement(const char* value) const
{
    return TiXmlHandle(node ? node->FirstChildElement(value) : 0);
}

TiXmlHandle TiXmlHandle::Child(int count) const
{
    TiXmlNode* child = node ? node->FirstChild() : 0;
    for (int i = 0; child && i < count; ++i)
        child = child->NextSibling();
    return TiXmlHandle(child);
}

TiXmlHandle TiXmlHandle::Child(const char* value, int count) const
{
    TiXmlNode* child = node ? node->FirstChild(value) : 0;
    for (int i = 0; child && i < count; ++i)
        child = child->NextSibling(value);
    return TiXmlHandle(child);
}

TiXmlHandle TiXmlHandle::ChildElement(int count) const
{
    TiXmlElement* child = node ? node->FirstChildElement() : 0;
    for (int i = 0; child && i < count; ++i)
        child = child->NextSiblingElement();
    return TiXmlHandle(child);
}

TiXmlHandle TiXmlHandle::ChildElement(const char* value, int count) const
{
    TiXmlElement* child = node ? node->FirstChildElement(value) : 0;
    for (int i = 0; child && i < count; ++i)
        child = child->NextSiblingElement(value);
    return TiXmlHandle(child);
}

// ---- the document, and the reader on OXP ----

TiXmlDocument::TiXmlDocument()
    : TiXmlNode(TiXmlNode::TINYXML_DOCUMENT), error(false), errorId(0), tabsize(4)
{
    ClearError();
}

TiXmlDocument::TiXmlDocument(const char* documentName)
    : TiXmlNode(TiXmlNode::TINYXML_DOCUMENT), error(false), errorId(0), tabsize(4)
{
    SetValue(documentName);
    ClearError();
}

TiXmlDocument::TiXmlDocument(const std::string& documentName)
    : TiXmlNode(TiXmlNode::TINYXML_DOCUMENT), error(false), errorId(0), tabsize(4)
{
    value = documentName;
    ClearError();
}

TiXmlDocument::TiXmlDocument(const TiXmlDocument& copy)
    : TiXmlNode(TiXmlNode::TINYXML_DOCUMENT), error(false), errorId(0), tabsize(4)
{
    copy.CopyTo(this);
}

TiXmlDocument& TiXmlDocument::operator=(const TiXmlDocument& copy)
{
    if (&copy != this)
    {
        Clear();
        copy.CopyTo(this);
    }
    return *this;
}

void TiXmlDocument::CopyTo(TiXmlDocument* target) const
{
    TiXmlNode::CopyTo(target);
    target->error = error;
    target->errorId = errorId;
    target->errorDesc = errorDesc;
    target->tabsize = tabsize;
    target->errorLocation = errorLocation;
    for (const TiXmlNode* n = firstChild; n; n = n->NextSibling())
        target->LinkEndChild(n->Clone());
}

TiXmlNode* TiXmlDocument::Clone() const
{
    TiXmlDocument* clone = new TiXmlDocument();
    CopyTo(clone);
    return clone;
}

bool TiXmlDocument::Accept(TiXmlVisitor* visitor) const
{
    if (visitor->VisitEnter(*this))
    {
        for (const TiXmlNode* n = FirstChild(); n; n = n->NextSibling())
            if (!n->Accept(visitor))
                break;
    }
    return visitor->VisitExit(*this);
}

void TiXmlDocument::SetError(int err, const char* desc, int row, int col)
{
    // the first error is the one kept
    if (error)
        return;
    error = true;
    errorId = err;
    errorDesc = desc ? desc : "";
    errorLocation.row = row;
    errorLocation.col = col;
}

// Builds the tree from OXP's events: text is gathered until the next
// markup, then kept unless it is only white space
struct TiXmlOxpReader
{
    TiXmlDocument* doc;
    TiXmlNode* current;
    std::string text;
    bool textIsCdata;
    oxp_parser* parser;
    bool failed;
    std::string message;
    unsigned long line, column;

    void FlushText()
    {
        if (text.empty())
            return;
        if (textIsCdata)
        {
            TiXmlText* t = new TiXmlText(text);
            t->SetCDATA(true);
            current->LinkEndChild(t);
        }
        else
        {
            bool blank = true;
            for (size_t i = 0; i < text.size() && blank; ++i)
                blank = isspace((unsigned char)text[i]) != 0;
            if (!blank)
            {
                if (TiXmlBase::IsWhiteSpaceCondensed())
                {
                    // trim, and fold each run of white space to one space
                    std::string folded;
                    bool space = false;
                    for (size_t i = 0; i < text.size(); ++i)
                    {
                        unsigned char c = (unsigned char)text[i];
                        if (isspace(c))
                            space = true;
                        else
                        {
                            if (space && !folded.empty())
                                folded += ' ';
                            space = false;
                            folded += (char)c;
                        }
                    }
                    text.swap(folded);
                }
                current->LinkEndChild(new TiXmlText(text));
            }
        }
        text.clear();
        textIsCdata = false;
    }

    static TiXmlOxpReader* R(void* ud) { return static_cast<TiXmlOxpReader*>(ud); }

    static void StartDocument(void* ud, const char* version, const char* encoding, int standalone)
    {
        if (!version)
            return;
        TiXmlOxpReader* r = R(ud);
        r->doc->LinkEndChild(new TiXmlDeclaration(version, encoding ? encoding : "",
                                                  standalone < 0 ? "" : (standalone ? "yes" : "no")));
    }

    static void Doctype(void* ud, const char* name, const char* public_id, const char* system_id, int)
    {
        TiXmlOxpReader* r = R(ud);
        std::string v = std::string("!DOCTYPE ") + (name ? name : "");
        if (public_id)
            v += std::string(" PUBLIC \"") + public_id + "\" \"" + (system_id ? system_id : "") + "\"";
        else if (system_id)
            v += std::string(" SYSTEM \"") + system_id + "\"";
        TiXmlUnknown* u = new TiXmlUnknown();
        u->SetValue(v);
        r->doc->LinkEndChild(u);
    }

    static void StartElement(void* ud, const char* local, const char*, const char*,
                             int, const oxp_ns*, int nattrs, const oxp_attr* attrs)
    {
        TiXmlOxpReader* r = R(ud);
        r->FlushText();
        TiXmlElement* e = new TiXmlElement(local);
        e->location.row = (int)oxp_line(r->parser) - 1;
        e->location.col = (int)oxp_column(r->parser) - 1;
        for (int i = 0; i < nattrs; ++i)
            if (!attrs[i].defaulted)
                e->AppendAttribute(attrs[i].local, attrs[i].value);
        r->current->LinkEndChild(e);
        r->current = e;
    }

    static void EndElement(void* ud, const char*, const char*, const char*)
    {
        TiXmlOxpReader* r = R(ud);
        r->FlushText();
        if (r->current->Parent())
            r->current = r->current->Parent();
    }

    static void Characters(void* ud, const char* t, size_t len)
    {
        TiXmlOxpReader* r = R(ud);
        if (r->textIsCdata)
            r->FlushText();
        r->text.append(t, len);
    }

    static void Cdata(void* ud, const char* t, size_t len)
    {
        TiXmlOxpReader* r = R(ud);
        if (!r->textIsCdata)
            r->FlushText();
        r->textIsCdata = true;
        r->text.append(t, len);
    }

    static void Comment(void* ud, const char* t)
    {
        TiXmlOxpReader* r = R(ud);
        r->FlushText();
        r->current->LinkEndChild(new TiXmlComment(t));
    }

    static void Pi(void* ud, const char* target, const char* data)
    {
        TiXmlOxpReader* r = R(ud);
        r->FlushText();
        TiXmlUnknown* u = new TiXmlUnknown();
        std::string v = std::string("?") + target;
        if (data && *data)
            v += std::string(" ") + data;
        u->SetValue(v + "?");
        r->current->LinkEndChild(u);
    }

    static void Error(void* ud, oxp_level level, oxp_code, const char* msg, unsigned long l, unsigned long c)
    {
        TiXmlOxpReader* r = R(ud);
        if (level != OXP_FATAL || r->failed)
            return;
        r->failed = true;
        r->message = msg ? msg : "";
        r->line = l;
        r->column = c;
    }

    // one pass; returns the code OXP stopped with
    oxp_code Run(const char* p, size_t len, unsigned options)
    {
        oxp_handler h;
        memset(&h, 0, sizeof h);
        h.start_document = StartDocument;
        h.doctype = Doctype;
        h.start_element = StartElement;
        h.end_element = EndElement;
        h.characters = Characters;
        h.cdata = Cdata;
        h.comment = Comment;
        h.pi = Pi;
        h.error = Error;
        current = doc;
        text.clear();
        textIsCdata = false;
        failed = false;
        line = column = 0;
        parser = oxp_new(&h, this, 0);
        if (!parser)
            return OXP_E_NOMEM;
        oxp_set_options(parser, OXP_OPT_NO_NAMESPACES | options);
        oxp_code rc = oxp_parse(parser, p, len, 1);
        if (rc == OXP_OK)
            FlushText();
        oxp_free(parser);
        parser = 0;
        return rc;
    }
};

// TinyXML read some documents that are not well-formed, and a few of them
// are about (one of Code::Blocks' own templates has "--" inside a
// comment). When OXP refuses a document, it is read again with those two
// mended: "--" inside a comment becomes "- -", and an "&" that starts no
// reference becomes "&amp;".
static std::string Mend(const char* p, size_t len)
{
    std::string out;
    out.reserve(len + 64);
    size_t i = 0;
    while (i < len)
    {
        if (len - i >= 4 && memcmp(p + i, "<!--", 4) == 0)
        {
            const char* end = 0;
            for (size_t j = i + 4; j + 3 <= len; ++j)
                if (memcmp(p + j, "-->", 3) == 0)
                {
                    end = p + j;
                    break;
                }
            size_t stop = end ? (size_t)(end - p) : len;
            out.append("<!--");
            for (size_t j = i + 4; j < stop; ++j)
            {
                out += p[j];
                if (p[j] == '-' && (j + 1 == stop || p[j + 1] == '-'))
                    out += ' ';
            }
            if (end)
                out.append("-->");
            i = end ? stop + 3 : len;
            continue;
        }
        if (p[i] == '&')
        {
            size_t j = i + 1;
            if (j < len && p[j] == '#')
                ++j;
            while (j < len && (isalnum((unsigned char)p[j]) || p[j] == '_' || p[j] == '-' || p[j] == '.' || p[j] == ':'))
                ++j;
            if (!(j < len && p[j] == ';' && j > i + 1))
            {
                out.append("&amp;");
                ++i;
                continue;
            }
        }
        out += p[i++];
    }
    return out;
}

const char* TiXmlDocument::Parse(const char* p, TiXmlParsingData*, TiXmlEncoding)
{
    ClearError();
    if (!p || !*p)
    {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, "Error document empty.", 0, 0);
        return 0;
    }
    size_t len = strlen(p);
    TiXmlNode* keep = lastChild;
    TiXmlOxpReader r;
    r.doc = this;
    oxp_code rc = r.Run(p, len, 0);
    if (rc == OXP_E_ENCODING)
    {
        // an encoding OXP does not know: read the bytes as UTF-8, as
        // TinyXML did, dropping what the first pass built
        while (lastChild != keep)
            RemoveChild(lastChild);
        rc = r.Run(p, len, OXP_OPT_IGNORE_DECL_ENCODING);
    }
    if (rc != OXP_OK)
    {
        std::string mended = Mend(p, len);
        if (mended != std::string(p, len))
        {
            std::string first = r.message;
            unsigned long l = r.line, c = r.column;
            while (lastChild != keep)
                RemoveChild(lastChild);
            rc = r.Run(mended.c_str(), mended.size(), 0);
            if (rc != OXP_OK)
            {
                // report where the document itself went wrong
                r.message = first;
                r.line = l;
                r.column = c;
            }
        }
    }
    if (rc != OXP_OK)
    {
        while (lastChild != keep)
            RemoveChild(lastChild);
        std::string desc = r.message.empty() ? std::string(oxp_code_name(rc)) : r.message;
        SetError(TIXML_ERROR, desc.c_str(), r.line ? (int)r.line - 1 : 0, r.column ? (int)r.column - 1 : 0);
        return 0;
    }
    if (!FirstChildElement())
    {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, "Error document empty.", 0, 0);
        return 0;
    }
    return p + len;
}

bool TiXmlDocument::LoadFile(TiXmlEncoding encoding)
{
    return LoadFile(Value(), encoding);
}

bool TiXmlDocument::SaveFile() const
{
    return SaveFile(Value());
}

bool TiXmlDocument::LoadFile(const char* filename, TiXmlEncoding encoding)
{
    value = filename ? filename : "";
    FILE* file = filename ? fopen(filename, "rb") : 0;
    if (!file)
    {
        SetError(TIXML_ERROR_OPENING_FILE, "Error opening file.", 0, 0);
        return false;
    }
    bool ok = LoadFile(file, encoding);
    fclose(file);
    return ok;
}

bool TiXmlDocument::LoadFile(FILE* file, TiXmlEncoding encoding)
{
    if (!file)
    {
        SetError(TIXML_ERROR_OPENING_FILE, "Error opening file.", 0, 0);
        return false;
    }
    Clear();
    std::string data;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, file)) > 0)
        data.append(buf, n);
    if (data.empty())
    {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, "Error document empty.", 0, 0);
        return false;
    }
    if (data.find('\0') != std::string::npos)
    {
        SetError(TIXML_ERROR_EMBEDDED_NULL, "Embedded null character.", 0, 0);
        return false;
    }
    Parse(data.c_str(), 0, encoding);
    return !Error();
}

bool TiXmlDocument::SaveFile(const char* filename) const
{
    FILE* fp = filename ? fopen(filename, "w") : 0;
    if (!fp)
        return false;
    bool ok = SaveFile(fp);
    fclose(fp);
    return ok;
}

bool TiXmlDocument::SaveFile(FILE* fp) const
{
    TiXmlPrinter printer;
    Accept(&printer);
    return fwrite(printer.CStr(), 1, printer.Size(), fp) == printer.Size() && !ferror(fp);
}

// ---- streams ----

std::istream& operator>>(std::istream& in, TiXmlNode& base)
{
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    TiXmlDocument* doc = base.ToDocument();
    if (doc)
        doc->Parse(data.c_str());
    return in;
}

std::ostream& operator<<(std::ostream& out, const TiXmlNode& base)
{
    TiXmlPrinter printer;
    printer.SetStreamPrinting();
    base.Accept(&printer);
    out << printer.Str();
    return out;
}

std::string& operator<<(std::string& out, const TiXmlNode& base)
{
    TiXmlPrinter printer;
    printer.SetStreamPrinting();
    base.Accept(&printer);
    out.append(printer.Str());
    return out;
}
