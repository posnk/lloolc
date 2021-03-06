#include <string>
#include <iostream>
#include <cstdlib>
#include "ast.h"

void StorageClassNode::addToDeclaration(DeclarationASTNode &decl)
{
	decl.storageClasses.push_back(storageClass);
}

void TypeQualifierNode::addToDeclaration(DeclarationASTNode &decl)
{
	decl.typeQualifiers.push_back(typeQualifier);
}

void TypeSpecNode::addToDeclaration(DeclarationASTNode &decl)
{
	decl.typeSpecifiers.push_back(this->clone());
}

DeclarationASTNode::~DeclarationASTNode()
{
	for(DeclarationSpecList::iterator it = specifiers->begin();
		 it != specifiers->end(); 
		++it) 
		delete *it;
	delete specifiers;
	for(DeclaratorList::iterator it = declarators->begin();
		 it != declarators->end(); 
		++it) 
		delete *it;
	delete declarators;
}

void DeclarationASTNode::addDeclarationSpecifiers( void )
{
	if (declSpecAdded)
		return;
	declSpecAdded = true;
	for(DeclarationSpecList::iterator it = specifiers->begin();
		it != specifiers->end(); 
		++it)
		(*it)->addToDeclaration(*this); 
}

BaseType *DeclarationASTNode::getBaseType( void ) {
	
	if (baseType != nullptr)
		return baseType;
	
	addDeclarationSpecifiers();

	baseType = nullptr; 
	TypeSpecNode *baseTypeSpec = nullptr;
	for (TypeSpecList::iterator it = typeSpecifiers.begin();
		it != typeSpecifiers.end();
		++it) {
		TypeSpecNode *ts = *it;
		if (ts->isRootType()) {
			baseType = ts->getType(nullptr, typeQualifiers);
			baseTypeSpec = ts;
			break;
		}			
	}
	
	if (baseType == nullptr)
		baseType = new BuiltinType(typeQualifiers,
					32, false, true);//int32_t (int)
	
	for (TypeSpecList::iterator it = typeSpecifiers.begin();
		it != typeSpecifiers.end();
		++it) {
		TypeSpecNode *ts = *it;
		if (ts == baseTypeSpec)
			continue;
		baseType = ts->getType( baseType, typeQualifiers );
	}

	return baseType;
}

void DeclarationASTNode::declareSymbols()
{
	getBaseType();
	for (DeclaratorList::iterator it = declarators->begin();
		it != declarators->end();
		++it) {
		DeclaratorASTNode *decl = *it;
		decl->makeSymbol(*baseType);
	}
}

TypeSpecNode *BuiltinTypeSpecNode::clone()
{
	return new BuiltinTypeSpecNode( *this );
}

TypeSpecNode *NamedTypeSpecNode::clone()
{
	return new NamedTypeSpecNode( *this );
}

BaseType *NamedTypeSpecNode::getType( BaseType* OldType, 
					  TypeQualifierList &Quals ){
	if ( OldType != nullptr ) {
		//TODO: Proper error handling
		cerr << "Invalid use of named type as additional type specifier.\n";
		exit(EXIT_FAILURE);
		return OldType;
	}
	return new BuiltinType(Quals,0, false, false);
	//TODO: Decide on typename tagging for type checks
	//TODO: Implement typedef storage lookup here
}

bool BuiltinTypeSpecNode::isRootType( void  ){
	switch ( specifier ) {
		case TS_VOID:
		case TS_CHAR:
		case TS_INT:
		case TS_FLOAT:
		case TS_DOUBLE:
			return true;
		case TS_LONG:
		case TS_SHORT:
		case TS_UNSIGNED:
		case TS_SIGNED:
			return false;
		default:
			//TODO: Proper error handling
			cerr << "Logic Error: Unknown builtin type specifier\n";
			exit(EXIT_FAILURE);
			return false;
	}
}

void builtinTypeOverspecified( void ){
	//TODO: Proper error handling
	cerr << "Overspecified builtin type\n";
	exit(EXIT_FAILURE);
}

void builtinTypeUnspecified( void ){
	//TODO: Proper error handling
	cerr << "Unspecified builtin type\n";
	exit(EXIT_FAILURE);
}

BaseType *BuiltinTypeSpecNode::getType ( BaseType* OldType, 
					 TypeQualifierList &Quals ) {
	switch ( specifier ) {
		/* Root Types */
		case TS_VOID:
			if ( OldType != nullptr ) {
				builtinTypeOverspecified();
				return OldType;
			}
			return new BuiltinType(Quals, 0, false, false);
		case TS_CHAR:
			if ( OldType != nullptr ) {
				builtinTypeOverspecified();
				return OldType;
			}
			return new BuiltinType(Quals, 8, false, false);
		case TS_INT:
			if ( OldType != nullptr ) {
				builtinTypeOverspecified();
				return OldType;
			}
			return new BuiltinType(Quals, 32, false, true);
		case TS_FLOAT:
			if ( OldType != nullptr ) {
				builtinTypeOverspecified();
				return OldType;
			}
			return new BuiltinType(Quals, 32, true);
		case TS_DOUBLE:
			if ( OldType != nullptr ) {
				builtinTypeOverspecified();
				return OldType;
			}
			return new BuiltinType(Quals, 64, true);
		/* Modifier Types */
		case TS_LONG:
			if ( OldType == nullptr ) {
				builtinTypeUnspecified();
				return nullptr;
			}
			OldType->enlargen();
			return OldType;
		case TS_SHORT:
			if ( OldType == nullptr ) {
				builtinTypeUnspecified();
				return nullptr;
			}
			OldType->shrink();
			return OldType;
		case TS_SIGNED:
			if ( OldType == nullptr ) {
				builtinTypeUnspecified();
				return nullptr;
			}
			OldType->makeSigned();
			return OldType;
		case TS_UNSIGNED:
			if ( OldType == nullptr ) {
				builtinTypeUnspecified();
				return nullptr;
			}
			OldType->makeUnsigned();
			return OldType;
		
		default:
			//TODO: Proper error handling
			cerr << "Logic error: Unknown type specifier!\n";
			exit(EXIT_FAILURE);
			return nullptr;	

	}
}

Type *PointerSpecASTNode::generateType( Type &targetType )
{
	Type *ourType = new PointerType(*qualifiers, targetType);
	if ( pointerFrom != nullptr ) {
		Type *subType = pointerFrom->generateType(*ourType);
		delete ourType;
		return subType;
	}
	return ourType;
}

Symbol *SymbolDeclaratorNode::makeSymbol( Type &outerType )
{
	cout << "Declaring symbol " << name << " as " << outerType.toString() << "\n";
	//TODO: Create symbol
	return nullptr;
}

Symbol *PointerDeclaratorNode::makeSymbol( Type &outerType )
{
	Type *ptrType = pointer->generateType( outerType );
	Symbol *sym = inner->makeSymbol( *ptrType );
	delete ptrType;
	return sym;
}

Symbol *ArrayDeclaratorNode::makeSymbol( Type &outerType )
{
	Type *ourType = new ArrayType( outerType, sizeExpression->evaluate() );
	Symbol *sym = inner->makeSymbol( *ourType );
	delete ourType;
	return sym;
}

/*
Symbol *FunctionDeclaratorNode::makeSymbol( TypeNode *outerType )
{
	//TODO: OuterType is actually the return type here
	//TypeNode *ourType = new FunctionTypeNode ( outerType, 
	cerr << "Feature not yet implemented: function declarations\n";
	return nullptr;
}
*/

