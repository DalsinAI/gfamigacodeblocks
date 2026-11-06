// The TinyXML programming interface that Code::Blocks is written against,
// on the Team's own XML parser, OXP (gfopenxml). This is our own code: the
// tree, the printer and the reader on OXP's events. The build puts it in
// place of the TinyXML copy in Code::Blocks' source, so the program has one
// XML parser, ours. Only the parts Code::Blocks uses are here, with the
// same names, signatures and output format, so its project, workspace and
// configuration files read and write as they do everywhere else.
// Copyright (c) 2026 AmiCradle Team
#ifndef CB_TINYXML_OXP_H
#define CB_TINYXML_OXP_H

#include <stdio.h>
#include <string>
#include <iosfwd>

#ifndef TIXML_USE_STL
#define TIXML_USE_STL
#endif
#define TIXML_STRING std::string

#ifndef DLLIMPORT
#define DLLIMPORT
#endif

class TiXmlDocument;
class TiXmlElement;
class TiXmlComment;
class TiXmlUnknown;
class TiXmlAttribute;
class TiXmlText;
class TiXmlDeclaration;
class TiXmlParsingData;

const int TIXML_MAJOR_VERSION = 2;
const int TIXML_MINOR_VERSION = 6;
const int TIXML_PATCH_VERSION = 2;

enum
{
    TIXML_SUCCESS,
    TIXML_NO_ATTRIBUTE,
    TIXML_WRONG_TYPE
};

enum TiXmlEncoding
{
    TIXML_ENCODING_UNKNOWN,
    TIXML_ENCODING_UTF8,
    TIXML_ENCODING_LEGACY
};
const TiXmlEncoding TIXML_DEFAULT_ENCODING = TIXML_ENCODING_UNKNOWN;

struct TiXmlCursor
{
    TiXmlCursor() { Clear(); }
    void Clear() { row = col = -1; }
    int row;
    int col;
};

// Walks a tree: Accept calls these in document order
class DLLIMPORT TiXmlVisitor
{
public:
    virtual ~TiXmlVisitor() {}
    virtual bool VisitEnter(const TiXmlDocument&) { return true; }
    virtual bool VisitExit(const TiXmlDocument&) { return true; }
    virtual bool VisitEnter(const TiXmlElement&, const TiXmlAttribute*) { return true; }
    virtual bool VisitExit(const TiXmlElement&) { return true; }
    virtual bool Visit(const TiXmlDeclaration&) { return true; }
    virtual bool Visit(const TiXmlText&) { return true; }
    virtual bool Visit(const TiXmlComment&) { return true; }
    virtual bool Visit(const TiXmlUnknown&) { return true; }
};

class DLLIMPORT TiXmlBase
{
public:
    TiXmlBase() : userData(0) {}
    virtual ~TiXmlBase() {}

    virtual void Print(FILE* cfile, int depth) const = 0;

    // false keeps text as it was read; true trims it and folds runs of
    // white space to one space (whitespace-only text is never kept)
    static void SetCondenseWhiteSpace(bool condense) { condenseWhiteSpace = condense; }
    static bool IsWhiteSpaceCondensed() { return condenseWhiteSpace; }

    int Row() const { return location.row + 1; }
    int Column() const { return location.col + 1; }

    void SetUserData(void* user) { userData = user; }
    void* GetUserData() { return userData; }
    const void* GetUserData() const { return userData; }

    // the five predefined entities, and characters below 32 as &#xNN;
    static void EncodeString(const std::string& str, std::string* out);

    enum
    {
        TIXML_NO_ERROR = 0,
        TIXML_ERROR,
        TIXML_ERROR_OPENING_FILE,
        TIXML_ERROR_PARSING_ELEMENT,
        TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME,
        TIXML_ERROR_READING_ELEMENT_VALUE,
        TIXML_ERROR_READING_ATTRIBUTES,
        TIXML_ERROR_PARSING_EMPTY,
        TIXML_ERROR_READING_END_TAG,
        TIXML_ERROR_PARSING_UNKNOWN,
        TIXML_ERROR_PARSING_COMMENT,
        TIXML_ERROR_PARSING_DECLARATION,
        TIXML_ERROR_DOCUMENT_EMPTY,
        TIXML_ERROR_EMBEDDED_NULL,
        TIXML_ERROR_PARSING_CDATA,
        TIXML_ERROR_DOCUMENT_TOP_ONLY,
        TIXML_ERROR_STRING_COUNT
    };

protected:
    TiXmlCursor location;
    void* userData;
    static bool condenseWhiteSpace;

private:
    TiXmlBase(const TiXmlBase&);
    void operator=(const TiXmlBase&);
};

class DLLIMPORT TiXmlNode : public TiXmlBase
{
    friend class TiXmlDocument;
    friend class TiXmlElement;
    friend struct TiXmlOxpReader;

public:
    enum NodeType
    {
        TINYXML_DOCUMENT,
        TINYXML_ELEMENT,
        TINYXML_COMMENT,
        TINYXML_UNKNOWN,
        TINYXML_TEXT,
        TINYXML_DECLARATION,
        TINYXML_TYPECOUNT
    };

    virtual ~TiXmlNode();

    const char* Value() const { return value.c_str(); }
    const std::string& ValueStr() const { return value; }
    const std::string& ValueTStr() const { return value; }
    void SetValue(const char* _value) { value = _value ? _value : ""; }
    void SetValue(const std::string& _value) { value = _value; }

    // deletes every child
    void Clear();

    TiXmlNode* Parent() { return parent; }
    const TiXmlNode* Parent() const { return parent; }

    const TiXmlNode* FirstChild() const { return firstChild; }
    TiXmlNode* FirstChild() { return firstChild; }
    const TiXmlNode* FirstChild(const char* value) const;
    TiXmlNode* FirstChild(const char* _value) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).FirstChild(_value)); }
    const TiXmlNode* LastChild() const { return lastChild; }
    TiXmlNode* LastChild() { return lastChild; }
    const TiXmlNode* LastChild(const char* value) const;
    TiXmlNode* LastChild(const char* _value) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).LastChild(_value)); }
    const TiXmlNode* FirstChild(const std::string& _value) const { return FirstChild(_value.c_str()); }
    TiXmlNode* FirstChild(const std::string& _value) { return FirstChild(_value.c_str()); }
    const TiXmlNode* LastChild(const std::string& _value) const { return LastChild(_value.c_str()); }
    TiXmlNode* LastChild(const std::string& _value) { return LastChild(_value.c_str()); }

    // the child after previous (the first when previous is null)
    const TiXmlNode* IterateChildren(const TiXmlNode* previous) const;
    TiXmlNode* IterateChildren(const TiXmlNode* previous) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).IterateChildren(previous)); }
    const TiXmlNode* IterateChildren(const char* value, const TiXmlNode* previous) const;
    TiXmlNode* IterateChildren(const char* _value, const TiXmlNode* previous) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).IterateChildren(_value, previous)); }
    const TiXmlNode* IterateChildren(const std::string& _value, const TiXmlNode* previous) const { return IterateChildren(_value.c_str(), previous); }
    TiXmlNode* IterateChildren(const std::string& _value, const TiXmlNode* previous) { return IterateChildren(_value.c_str(), previous); }

    // a copy of addThis as the last child
    TiXmlNode* InsertEndChild(const TiXmlNode& addThis);
    // addThis itself as the last child: the tree owns it from now on
    TiXmlNode* LinkEndChild(TiXmlNode* addThis);
    TiXmlNode* InsertBeforeChild(TiXmlNode* beforeThis, const TiXmlNode& addThis);
    TiXmlNode* InsertAfterChild(TiXmlNode* afterThis, const TiXmlNode& addThis);
    TiXmlNode* ReplaceChild(TiXmlNode* replaceThis, const TiXmlNode& withThis);
    bool RemoveChild(TiXmlNode* removeThis);

    const TiXmlNode* PreviousSibling() const { return prev; }
    TiXmlNode* PreviousSibling() { return prev; }
    const TiXmlNode* PreviousSibling(const char*) const;
    TiXmlNode* PreviousSibling(const char* _prev) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).PreviousSibling(_prev)); }
    const TiXmlNode* PreviousSibling(const std::string& _value) const { return PreviousSibling(_value.c_str()); }
    TiXmlNode* PreviousSibling(const std::string& _value) { return PreviousSibling(_value.c_str()); }
    const TiXmlNode* NextSibling(const std::string& _value) const { return NextSibling(_value.c_str()); }
    TiXmlNode* NextSibling(const std::string& _value) { return NextSibling(_value.c_str()); }
    const TiXmlNode* NextSibling() const { return next; }
    TiXmlNode* NextSibling() { return next; }
    const TiXmlNode* NextSibling(const char*) const;
    TiXmlNode* NextSibling(const char* _next) { return const_cast<TiXmlNode*>(static_cast<const TiXmlNode&>(*this).NextSibling(_next)); }

    const TiXmlElement* NextSiblingElement() const;
    TiXmlElement* NextSiblingElement() { return const_cast<TiXmlElement*>(static_cast<const TiXmlNode&>(*this).NextSiblingElement()); }
    const TiXmlElement* NextSiblingElement(const char*) const;
    TiXmlElement* NextSiblingElement(const char* _next) { return const_cast<TiXmlElement*>(static_cast<const TiXmlNode&>(*this).NextSiblingElement(_next)); }
    const TiXmlElement* NextSiblingElement(const std::string& _value) const { return NextSiblingElement(_value.c_str()); }
    TiXmlElement* NextSiblingElement(const std::string& _value) { return NextSiblingElement(_value.c_str()); }

    const TiXmlElement* FirstChildElement() const;
    TiXmlElement* FirstChildElement() { return const_cast<TiXmlElement*>(static_cast<const TiXmlNode&>(*this).FirstChildElement()); }
    const TiXmlElement* FirstChildElement(const char* _value) const;
    TiXmlElement* FirstChildElement(const char* _value) { return const_cast<TiXmlElement*>(static_cast<const TiXmlNode&>(*this).FirstChildElement(_value)); }
    const TiXmlElement* FirstChildElement(const std::string& _value) const { return FirstChildElement(_value.c_str()); }
    TiXmlElement* FirstChildElement(const std::string& _value) { return FirstChildElement(_value.c_str()); }

    int Type() const { return type; }

    const TiXmlDocument* GetDocument() const;
    TiXmlDocument* GetDocument() { return const_cast<TiXmlDocument*>(static_cast<const TiXmlNode&>(*this).GetDocument()); }

    bool NoChildren() const { return !firstChild; }

    virtual const TiXmlDocument* ToDocument() const { return 0; }
    virtual const TiXmlElement* ToElement() const { return 0; }
    virtual const TiXmlComment* ToComment() const { return 0; }
    virtual const TiXmlUnknown* ToUnknown() const { return 0; }
    virtual const TiXmlText* ToText() const { return 0; }
    virtual const TiXmlDeclaration* ToDeclaration() const { return 0; }
    virtual TiXmlDocument* ToDocument() { return 0; }
    virtual TiXmlElement* ToElement() { return 0; }
    virtual TiXmlComment* ToComment() { return 0; }
    virtual TiXmlUnknown* ToUnknown() { return 0; }
    virtual TiXmlText* ToText() { return 0; }
    virtual TiXmlDeclaration* ToDeclaration() { return 0; }

    virtual TiXmlNode* Clone() const = 0;
    virtual bool Accept(TiXmlVisitor* visitor) const = 0;

protected:
    TiXmlNode(NodeType _type);
    void CopyTo(TiXmlNode* target) const;

    TiXmlNode* parent;
    NodeType type;
    TiXmlNode* firstChild;
    TiXmlNode* lastChild;
    std::string value;
    TiXmlNode* prev;
    TiXmlNode* next;

private:
    TiXmlNode(const TiXmlNode&);
    void operator=(const TiXmlNode&);
};

class DLLIMPORT TiXmlAttribute : public TiXmlBase
{
    friend class TiXmlElement;

public:
    TiXmlAttribute() : document(0), prev(0), next(0) {}
    TiXmlAttribute(const std::string& _name, const std::string& _value)
        : name(_name), value(_value), document(0), prev(0), next(0) {}
    TiXmlAttribute(const char* _name, const char* _value)
        : name(_name ? _name : ""), value(_value ? _value : ""), document(0), prev(0), next(0) {}

    const char* Name() const { return name.c_str(); }
    const char* Value() const { return value.c_str(); }
    const std::string& ValueStr() const { return value; }
    const std::string& NameTStr() const { return name; }
    int IntValue() const;
    double DoubleValue() const;

    int QueryIntValue(int* _value) const;
    int QueryDoubleValue(double* _value) const;

    void SetName(const char* _name) { name = _name ? _name : ""; }
    void SetValue(const char* _value) { value = _value ? _value : ""; }
    void SetIntValue(int _value);
    void SetDoubleValue(double _value);
    void SetName(const std::string& _name) { name = _name; }
    void SetValue(const std::string& _value) { value = _value; }

    const TiXmlAttribute* Next() const { return next; }
    TiXmlAttribute* Next() { return next; }
    const TiXmlAttribute* Previous() const { return prev; }
    TiXmlAttribute* Previous() { return prev; }

    bool operator==(const TiXmlAttribute& rhs) const { return rhs.name == name; }
    bool operator<(const TiXmlAttribute& rhs) const { return name < rhs.name; }
    bool operator>(const TiXmlAttribute& rhs) const { return name > rhs.name; }

    virtual void Print(FILE* cfile, int depth) const { Print(cfile, depth, 0); }
    void Print(FILE* cfile, int depth, std::string* str) const;

    void SetDocument(TiXmlDocument* doc) { document = doc; }

private:
    TiXmlAttribute(const TiXmlAttribute&);
    void operator=(const TiXmlAttribute&);

    std::string name;
    std::string value;
    TiXmlDocument* document;
    TiXmlAttribute* prev;
    TiXmlAttribute* next;
};

class DLLIMPORT TiXmlElement : public TiXmlNode
{
public:
    TiXmlElement(const char* in_value);
    TiXmlElement(const std::string& _value);
    TiXmlElement(const TiXmlElement&);
    TiXmlElement& operator=(const TiXmlElement& base);
    virtual ~TiXmlElement();

    const char* Attribute(const char* name) const;
    const char* Attribute(const char* name, int* i) const;
    const char* Attribute(const char* name, double* d) const;
    int QueryIntAttribute(const char* name, int* _value) const;
    int QueryUnsignedAttribute(const char* name, unsigned* _value) const;
    int QueryBoolAttribute(const char* name, bool* _value) const;
    int QueryDoubleAttribute(const char* name, double* _value) const;
    int QueryFloatAttribute(const char* name, float* _value) const
    {
        double d;
        int result = QueryDoubleAttribute(name, &d);
        if (result == TIXML_SUCCESS)
            *_value = (float)d;
        return result;
    }
    int QueryStringAttribute(const char* name, std::string* _value) const
    {
        const char* cstr = Attribute(name);
        if (!cstr)
            return TIXML_NO_ATTRIBUTE;
        *_value = cstr;
        return TIXML_SUCCESS;
    }
    int QueryValueAttribute(const std::string& name, std::string* outValue) const
    {
        return QueryStringAttribute(name.c_str(), outValue);
    }

    void SetAttribute(const char* name, const char* _value);
    const std::string* Attribute(const std::string& name) const;
    const std::string* Attribute(const std::string& name, int* i) const;
    const std::string* Attribute(const std::string& name, double* d) const;
    int QueryIntAttribute(const std::string& name, int* _value) const { return QueryIntAttribute(name.c_str(), _value); }
    int QueryDoubleAttribute(const std::string& name, double* _value) const { return QueryDoubleAttribute(name.c_str(), _value); }
    void SetAttribute(const std::string& name, const std::string& _value) { SetAttribute(name.c_str(), _value.c_str()); }
    void SetAttribute(const std::string& name, int _value) { SetAttribute(name.c_str(), _value); }
    void SetDoubleAttribute(const std::string& name, double value) { SetDoubleAttribute(name.c_str(), value); }
    void SetAttribute(const char* name, int value);
    void SetDoubleAttribute(const char* name, double value);

    void RemoveAttribute(const char* name);
    void RemoveAttribute(const std::string& name) { RemoveAttribute(name.c_str()); }

    const TiXmlAttribute* FirstAttribute() const { return firstAttr; }
    TiXmlAttribute* FirstAttribute() { return firstAttr; }
    const TiXmlAttribute* LastAttribute() const { return lastAttr; }
    TiXmlAttribute* LastAttribute() { return lastAttr; }

    // the text of the first child when that is text, otherwise null
    const char* GetText() const;

    virtual TiXmlNode* Clone() const;
    virtual void Print(FILE* cfile, int depth) const;

    virtual const TiXmlElement* ToElement() const { return this; }
    virtual TiXmlElement* ToElement() { return this; }

    virtual bool Accept(TiXmlVisitor* visitor) const;

    // the reader appends attributes in document order
    void AppendAttribute(const char* name, const char* value);

protected:
    void CopyTo(TiXmlElement* target) const;
    void ClearThis();

private:
    TiXmlAttribute* Find(const char* name) const;

    TiXmlAttribute* firstAttr;
    TiXmlAttribute* lastAttr;
};

class DLLIMPORT TiXmlComment : public TiXmlNode
{
public:
    TiXmlComment() : TiXmlNode(TiXmlNode::TINYXML_COMMENT) {}
    TiXmlComment(const char* _value) : TiXmlNode(TiXmlNode::TINYXML_COMMENT) { SetValue(_value); }
    TiXmlComment(const TiXmlComment& copy) : TiXmlNode(TiXmlNode::TINYXML_COMMENT) { copy.CopyTo(this); }
    TiXmlComment& operator=(const TiXmlComment& base) { Clear(); base.CopyTo(this); return *this; }
    virtual ~TiXmlComment() {}

    virtual TiXmlNode* Clone() const;
    virtual void Print(FILE* cfile, int depth) const;
    virtual const TiXmlComment* ToComment() const { return this; }
    virtual TiXmlComment* ToComment() { return this; }
    virtual bool Accept(TiXmlVisitor* visitor) const;
};

class DLLIMPORT TiXmlText : public TiXmlNode
{
public:
    TiXmlText(const char* initValue) : TiXmlNode(TiXmlNode::TINYXML_TEXT), cdata(false) { SetValue(initValue); }
    TiXmlText(const std::string& initValue) : TiXmlNode(TiXmlNode::TINYXML_TEXT), cdata(false) { SetValue(initValue); }
    TiXmlText(const TiXmlText& copy) : TiXmlNode(TiXmlNode::TINYXML_TEXT), cdata(false) { copy.CopyTo(this); }
    TiXmlText& operator=(const TiXmlText& base) { base.CopyTo(this); return *this; }
    virtual ~TiXmlText() {}

    virtual void Print(FILE* cfile, int depth) const;

    bool CDATA() const { return cdata; }
    void SetCDATA(bool _cdata) { cdata = _cdata; }

    virtual const TiXmlText* ToText() const { return this; }
    virtual TiXmlText* ToText() { return this; }
    virtual bool Accept(TiXmlVisitor* content) const;
    virtual TiXmlNode* Clone() const;

    // only white space
    bool Blank() const;

protected:
    void CopyTo(TiXmlText* target) const;

private:
    bool cdata;
};

class DLLIMPORT TiXmlDeclaration : public TiXmlNode
{
public:
    TiXmlDeclaration() : TiXmlNode(TiXmlNode::TINYXML_DECLARATION) {}
    TiXmlDeclaration(const std::string& _version, const std::string& _encoding, const std::string& _standalone)
        : TiXmlNode(TiXmlNode::TINYXML_DECLARATION), version(_version), encoding(_encoding), standalone(_standalone) {}
    TiXmlDeclaration(const char* _version, const char* _encoding, const char* _standalone)
        : TiXmlNode(TiXmlNode::TINYXML_DECLARATION),
          version(_version ? _version : ""), encoding(_encoding ? _encoding : ""), standalone(_standalone ? _standalone : "") {}
    TiXmlDeclaration(const TiXmlDeclaration& copy) : TiXmlNode(TiXmlNode::TINYXML_DECLARATION) { copy.CopyTo(this); }
    TiXmlDeclaration& operator=(const TiXmlDeclaration& copy) { Clear(); copy.CopyTo(this); return *this; }
    virtual ~TiXmlDeclaration() {}

    const char* Version() const { return version.c_str(); }
    const char* Encoding() const { return encoding.c_str(); }
    const char* Standalone() const { return standalone.c_str(); }

    virtual TiXmlNode* Clone() const;
    virtual void Print(FILE* cfile, int depth, std::string* str) const;
    virtual void Print(FILE* cfile, int depth) const { Print(cfile, depth, 0); }

    virtual const TiXmlDeclaration* ToDeclaration() const { return this; }
    virtual TiXmlDeclaration* ToDeclaration() { return this; }
    virtual bool Accept(TiXmlVisitor* visitor) const;

protected:
    void CopyTo(TiXmlDeclaration* target) const;

private:
    std::string version;
    std::string encoding;
    std::string standalone;
};

// Markup kept as it was, between < and >: a DOCTYPE or a processing
// instruction
class DLLIMPORT TiXmlUnknown : public TiXmlNode
{
public:
    TiXmlUnknown() : TiXmlNode(TiXmlNode::TINYXML_UNKNOWN) {}
    virtual ~TiXmlUnknown() {}
    TiXmlUnknown(const TiXmlUnknown& copy) : TiXmlNode(TiXmlNode::TINYXML_UNKNOWN) { copy.CopyTo(this); }
    TiXmlUnknown& operator=(const TiXmlUnknown& copy) { Clear(); copy.CopyTo(this); return *this; }

    virtual TiXmlNode* Clone() const;
    virtual void Print(FILE* cfile, int depth) const;
    virtual const TiXmlUnknown* ToUnknown() const { return this; }
    virtual TiXmlUnknown* ToUnknown() { return this; }
    virtual bool Accept(TiXmlVisitor* content) const;
};

class DLLIMPORT TiXmlDocument : public TiXmlNode
{
public:
    TiXmlDocument();
    TiXmlDocument(const char* documentName);
    TiXmlDocument(const std::string& documentName);
    TiXmlDocument(const TiXmlDocument& copy);
    TiXmlDocument& operator=(const TiXmlDocument& copy);
    virtual ~TiXmlDocument() {}

    bool LoadFile(TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
    bool SaveFile() const;
    bool LoadFile(const char* filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
    bool SaveFile(const char* filename) const;
    bool LoadFile(FILE*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
    bool SaveFile(FILE*) const;
    bool LoadFile(const std::string& filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING) { return LoadFile(filename.c_str(), encoding); }
    bool SaveFile(const std::string& filename) const { return SaveFile(filename.c_str()); }

    // reads a whole document (NUL-terminated); returns the end of it, or
    // null when it is not well-formed (Error() and the rest say why)
    virtual const char* Parse(const char* p, TiXmlParsingData* data = 0, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);

    const TiXmlElement* RootElement() const { return FirstChildElement(); }
    TiXmlElement* RootElement() { return FirstChildElement(); }

    bool Error() const { return error; }
    const char* ErrorDesc() const { return errorDesc.c_str(); }
    int ErrorId() const { return errorId; }
    int ErrorRow() const { return errorLocation.row + 1; }
    int ErrorCol() const { return errorLocation.col + 1; }

    void SetTabSize(int _tabsize) { tabsize = _tabsize; }
    int TabSize() const { return tabsize; }

    void ClearError()
    {
        error = false;
        errorId = 0;
        errorDesc = "";
        errorLocation.row = errorLocation.col = 0;
    }

    void Print() const { Print(stdout, 0); }
    virtual void Print(FILE* cfile, int depth = 0) const;

    void SetError(int err, const char* desc, int row, int col);

    virtual const TiXmlDocument* ToDocument() const { return this; }
    virtual TiXmlDocument* ToDocument() { return this; }
    virtual bool Accept(TiXmlVisitor* content) const;

protected:
    virtual TiXmlNode* Clone() const;

private:
    void CopyTo(TiXmlDocument* target) const;

    bool error;
    int errorId;
    std::string errorDesc;
    int tabsize;
    TiXmlCursor errorLocation;
};

// A pointer that may be null, so chains of lookups need no checks
class DLLIMPORT TiXmlHandle
{
public:
    TiXmlHandle(TiXmlNode* _node) { this->node = _node; }
    TiXmlHandle(const TiXmlHandle& ref) { this->node = ref.node; }
    TiXmlHandle operator=(const TiXmlHandle& ref) { if (&ref != this) this->node = ref.node; return *this; }

    TiXmlHandle FirstChild() const;
    TiXmlHandle FirstChild(const char* value) const;
    TiXmlHandle FirstChildElement() const;
    TiXmlHandle FirstChildElement(const char* value) const;
    TiXmlHandle Child(const char* value, int index) const;
    TiXmlHandle Child(int index) const;
    TiXmlHandle ChildElement(const char* value, int index) const;
    TiXmlHandle ChildElement(int index) const;
    TiXmlHandle FirstChild(const std::string& _value) const { return FirstChild(_value.c_str()); }
    TiXmlHandle FirstChildElement(const std::string& _value) const { return FirstChildElement(_value.c_str()); }
    TiXmlHandle Child(const std::string& _value, int index) const { return Child(_value.c_str(), index); }
    TiXmlHandle ChildElement(const std::string& _value, int index) const { return ChildElement(_value.c_str(), index); }

    TiXmlNode* ToNode() const { return node; }
    TiXmlElement* ToElement() const { return (node && node->ToElement()) ? node->ToElement() : 0; }
    TiXmlText* ToText() const { return (node && node->ToText()) ? node->ToText() : 0; }
    TiXmlUnknown* ToUnknown() const { return (node && node->ToUnknown()) ? node->ToUnknown() : 0; }
    TiXmlNode* Node() const { return ToNode(); }
    TiXmlElement* Element() const { return ToElement(); }
    TiXmlText* Text() const { return ToText(); }
    TiXmlUnknown* Unknown() const { return ToUnknown(); }

private:
    TiXmlNode* node;
};

// Writes a tree out as text, indented (four spaces and a newline unless
// set otherwise)
class DLLIMPORT TiXmlPrinter : public TiXmlVisitor
{
public:
    TiXmlPrinter() : depth(0), simpleTextPrint(false), indent("    "), lineBreak("\n") {}

    virtual bool VisitEnter(const TiXmlDocument& doc);
    virtual bool VisitExit(const TiXmlDocument& doc);
    virtual bool VisitEnter(const TiXmlElement& element, const TiXmlAttribute* firstAttribute);
    virtual bool VisitExit(const TiXmlElement& element);
    virtual bool Visit(const TiXmlDeclaration& declaration);
    virtual bool Visit(const TiXmlText& text);
    virtual bool Visit(const TiXmlComment& comment);
    virtual bool Visit(const TiXmlUnknown& unknown);

    void SetIndent(const char* _indent) { indent = _indent ? _indent : ""; }
    const char* Indent() { return indent.c_str(); }
    void SetLineBreak(const char* _lineBreak) { lineBreak = _lineBreak ? _lineBreak : ""; }
    const char* LineBreak() { return lineBreak.c_str(); }
    // all on one line: no indent and no line breaks
    void SetStreamPrinting() { indent = ""; lineBreak = ""; }
    const char* CStr() { return buffer.c_str(); }
    size_t Size() { return buffer.size(); }
    const std::string& Str() { return buffer; }

private:
    void DoIndent() { for (int i = 0; i < depth; ++i) buffer += indent; }
    void DoLineBreak() { buffer += lineBreak; }

    int depth;
    bool simpleTextPrint;
    std::string buffer;
    std::string indent;
    std::string lineBreak;
};

std::istream& operator>>(std::istream& in, TiXmlNode& base);
std::ostream& operator<<(std::ostream& out, const TiXmlNode& base);
std::string& operator<<(std::string& out, const TiXmlNode& base);

#endif
