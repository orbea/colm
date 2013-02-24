/*
 *  Copyright 2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "bytecode.h"
#include "parsedata.h"
#include <iostream>
#include <assert.h>

void LexJoin::varDecl( Compiler *pd, ObjectDef *objectDef )
{
	expr->varDecl( pd, objectDef );
}

void LexExpression::varDecl( Compiler *pd, ObjectDef *objectDef )
{
	switch ( type ) {
		case OrType: case IntersectType: case SubtractType:
		case StrongSubtractType:
			expression->varDecl( pd, objectDef );
			term->varDecl( pd, objectDef );
			break;
		case TermType:
			term->varDecl( pd, objectDef );
			break;
		case BuiltinType:
			break;
	}
}

void LexTerm::varDecl( Compiler *pd, ObjectDef *objectDef )
{
	switch ( type ) {
		case ConcatType:
		case RightStartType:
		case RightFinishType:
		case LeftType:
			term->varDecl( pd, objectDef );
			factorAug->varDecl( pd, objectDef );
			break;
		case FactorAugType:
			factorAug->varDecl( pd, objectDef );
			break;
	}
}

void LexFactorAug::varDecl( Compiler *pd, ObjectDef *objectDef )
{
	for ( ReCaptureVect::Iter re = reCaptureVect; re.lte(); re++ ) {
		if ( objectDef->checkRedecl( re->objField->name ) != 0 )
			error(re->objField->loc) << "label name \"" << re->objField->name << "\" already in use" << endp;

		/* Insert it into the map. */
		objectDef->insertField( re->objField->name, re->objField );
	}
}

void Compiler::varDeclaration()
{
	for ( NamespaceList::Iter n = namespaceList; n.lte(); n++ ) {
		for ( TokenDefListNs::Iter tok = n->tokenDefList; tok.lte(); tok++ ) {
			if ( tok->join != 0 )
				tok->join->varDecl( this, tok->objectDef );
		}
	}
}

LangEl *declareLangEl( Compiler *pd, Namespace *nspace, const String &data, LangEl::Type type )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "'" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert.. */
	LangEl *langEl = new LangEl( nspace, data, type );
	TypeMapEl *typeMapEl = new TypeMapEl( data, langEl );
	nspace->typeMap.insert( typeMapEl );
	pd->langEls.append( langEl );

	return langEl;
}

/* Does not map the new language element. */
LangEl *addLangEl( Compiler *pd, Namespace *nspace, const String &data, LangEl::Type type )
{
	LangEl *langEl = new LangEl( nspace, data, type );
	pd->langEls.append( langEl );
	return langEl;
}

void declareTypeAlias( Compiler *pd, Namespace *nspace, const String &data, TypeRef *typeRef )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "'" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert.. */
	TypeMapEl *typeMapEl = new TypeMapEl( data, typeRef );
	nspace->typeMap.insert( typeMapEl );
}

LangEl *findType( Compiler *pd, Namespace *nspace, const String &data )
{
	/* If the id is already in the dict, it will be placed in last found. If
	 * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );

	if ( inDict == 0 )
		error() << "'" << data << "' not declared as anything" << endp;

	return inDict->value;
}


void Compiler::declareBaseLangEls()
{
	/* Order here is important because we make assumptions about the inbuild
	 * language elements in the runtime. Note tokens are have identifiers set
	 * in an initial pass. */

	/* Make a "_notoken" language element. This element is used when a
	 * generation action fails to generate anything, but there is reverse code
	 * that needs to be associated with a language element. This allows us to
	 * always associate reverse code with the first language element produced
	 * after a generation action. */
	noTokenLangEl = declareLangEl( this, rootNamespace, "_notoken", LangEl::Term );
	noTokenLangEl->isIgnore = true;
	
	/* Make the "stream" language element */
	ptrLangEl = declareLangEl( this, rootNamespace, "ptr", LangEl::Term );
	boolLangEl = declareLangEl( this, rootNamespace, "bool", LangEl::Term );
	intLangEl = declareLangEl( this, rootNamespace, "int", LangEl::Term );
	strLangEl = declareLangEl( this, rootNamespace, "str", LangEl::Term );
	streamLangEl = declareLangEl( this, rootNamespace, "stream", LangEl::Term );
	ignoreLangEl = declareLangEl( this, rootNamespace, "il", LangEl::Term );

	/* Make the EOF language element. */
	eofLangEl = 0;

	/* Make the "any" language element */
	anyLangEl = declareLangEl( this, rootNamespace, "any", LangEl::NonTerm );
}


void Compiler::addProdRedObjectVar( ObjectDef *localFrame, LangEl *nonTerm )
{
	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, nonTerm );
	TypeRef *typeRef = TypeRef::cons( internal, prodNameUT );
	ObjectField *el = ObjectField::cons( internal, typeRef, "lhs" );

	el->isLhsEl = true;

	initLocalInstructions( el );

	localFrame->insertField( el->name, el );
}

void Compiler::addProdLHSLoad( Production *prod, CodeVect &code, long &insertPos )
{
	ObjectField *lhsField = prod->redBlock->localFrame->findField("lhs");
	assert( lhsField != 0 );

	CodeVect loads;
	if ( lhsField->beenReferenced ) {
		loads.append( IN_INIT_LHS_EL );
		loads.appendHalf( lhsField->offset );
	}

	code.insert( insertPos, loads );
	insertPos += loads.length();
}

void Compiler::addPushBackLHS( Production *prod, CodeVect &code, long &insertPos )
{
	CodeBlock *block = prod->redBlock;

	/* If the lhs tree is dirty then we will need to save off the old lhs
	 * before it gets modified. We want to avoid this for attribute
	 * modifications. The computation of dirtyTree should deal with this for
	 * us. */
	ObjectField *lhsField = block->localFrame->findField("lhs");
	assert( lhsField != 0 );

	if ( lhsField->beenReferenced ) {
		code.append( IN_STORE_LHS_EL );
		code.appendHalf( lhsField->offset );
	}
}

void Compiler::addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList )
{
	long position = 1;
	for ( ProdElList::Iter rhsEl = *prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			/* Use an offset of zero. For frame objects we compute the offset on
			 * demand. */
			String name( 8, "r%d", position );
			ObjectField *el = ObjectField::cons( InputLoc(), rhsEl->typeRef, name );
			rhsEl->objField = el;

			/* Right hand side elements are constant. */
			el->isConst = true;
			el->isRhsEl = true;

			/* Only ever fetch for reading since they are constant. */
			el->inGetR = IN_GET_LOCAL_R;

			localFrame->insertField( el->name, el );
		}
	}
}

void Compiler::addProdRHSLoads( Production *prod, CodeVect &code, long &insertPos )
{
	CodeVect loads;
	long elPos = 0;
	for ( ProdElList::Iter rhsEl = *prod->prodElList; rhsEl.lte(); rhsEl++, elPos++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			if ( rhsEl->objField->beenReferenced ) {
				loads.append ( IN_INIT_RHS_EL );
				loads.appendHalf( elPos );
				loads.appendHalf( rhsEl->objField->offset );
			}
		}
	}

	/* Insert and update the insert position. */
	code.insert( insertPos, loads );
	insertPos += loads.length();
}

void GenericType::declare( Compiler *pd, Namespace *nspace )
{
	//std::cout << "generic " << g->name << std::endl;

	LangEl *langEl = declareLangEl( pd, nspace, name, LangEl::NonTerm );

	/* Add one empty production. */
	ProdElList *emptyList = new ProdElList;
	//addProduction( g->loc, langEl, emptyList, false, 0, 0 );

	{
		LangEl *prodName = langEl;
		assert( prodName->type == LangEl::NonTerm );

		Production *newDef = Production::cons( InputLoc(), prodName, 
			emptyList, false, 0,
			pd->prodList.length(), prodName->defList.length() );
			
		prodName->defList.append( newDef );
		pd->prodList.append( newDef );
		newDef->predOf = 0;
	}

	langEl->generic = this;
	this->langEl = langEl;
}

void Namespace::declare( Compiler *pd )
{
	for ( GenericList::Iter g = genericList; g.lte(); g++ )
		g->declare( pd, this );

	for ( TokenInstanceListNs::Iter l = tokenInstanceList; l.lte(); l++ ) {
		if ( l->dupOf == 0 ) {
			if ( l->tokenDef->isLiteral ) {
				if ( l->isZero ) {
					l->tokenDef->tdLangEl = l->tokenRegion->ciLel;
					assert( l->tokenRegion->ciLel != 0 );
				}
				else {
					/* Original. Create a token for the literal. */
					LangEl *newLangEl = declareLangEl( pd, this, l->name, LangEl::Term );

					newLangEl->lit = l->literal;
					newLangEl->isLiteral = true;
					newLangEl->tokenInstance = l;

					l->tokenDef->tdLangEl = newLangEl;

					if ( l->tokenDef->noPreIgnore )
						newLangEl->noPreIgnore = true;
					if ( l->tokenDef->noPostIgnore )
						newLangEl->noPostIgnore = true;
				}
			}
		}
	}

	for ( ContextDefList::Iter c = contextDefList; c.lte(); c++ ) {
		LangEl *lel = declareLangEl( pd, this, c->name, LangEl::NonTerm );
		ProdElList *emptyList = new ProdElList;
		//addProduction( c->context->loc, c->name, emptyList, false, 0, 0 );

		{
			LangEl *prodName = lel;
			assert( prodName->type == LangEl::NonTerm );

			Production *newDef = Production::cons( loc, prodName, 
				emptyList, false, 0,
				pd->prodList.length(), prodName->defList.length() );
			
			prodName->defList.append( newDef );
			pd->prodList.append( newDef );
			newDef->predOf = 0;

			/* If the token has the same name as the region it is in, then also
			 * insert it into the symbol map for the parent region. */
			if ( strcmp( c->name, this->name ) == 0 ) {
				/* Insert the name into the top of the region stack after popping the
				 * region just created. We need it in the parent. */
				TypeMapEl *typeMapEl = new TypeMapEl( c->name, prodName );
				this->parentNamespace->typeMap.insert( typeMapEl );
			}
		}

		c->context->lel = lel;
		lel->contextDef = c->context;
		lel->objectDef = c->context->contextObjDef;
	}

	for ( TokenInstanceListNs::Iter t = tokenInstanceList; t.lte(); t++ ) {
		if ( t->dupOf == 0 ) {
			/* Literals already taken care of. */
			if ( ! t->tokenDef->isLiteral ) {
				/* Create the token. */
				LangEl *tokEl = declareLangEl( pd, this, t->name, LangEl::Term );
				tokEl->isIgnore = t->tokenDef->isIgnore;
				tokEl->transBlock = t->codeBlock;
				tokEl->objectDef = t->objectDef;
				tokEl->contextIn = t->contextIn;
				tokEl->tokenInstance = t;

				if ( t->tokenDef->noPreIgnore )
					tokEl->noPreIgnore = true;
				if ( t->tokenDef->noPostIgnore )
					tokEl->noPostIgnore = true;

				t->tokenDef->tdLangEl = tokEl;
			}
		}
	}

	for ( NtDefList::Iter n = ntDefList; n.lte(); n++ ) {
		/* Get the language element. */
		LangEl *langEl = declareLangEl( pd, this, n->name, LangEl::NonTerm );
		//$$->langEl = langEl;

		/* Get the language element. */
		langEl->objectDef = n->objectDef;
		langEl->reduceFirst = n->reduceFirst;
		langEl->contextIn = n->contextIn;
		langEl->defList.transfer( *n->defList );

		for ( LelDefList::Iter d = langEl->defList; d.lte(); d++ ) {
			d->prodName = langEl;

			if ( d->redBlock != 0 ) {
				pd->addProdRedObjectVar( d->redBlock->localFrame, langEl );
				pd->addProdRHSVars( d->redBlock->localFrame, d->prodElList );
			}

			/* References to the reduce item. */
		}
	}

	for ( TypeAliasList::Iter ta = typeAliasList; ta.lte(); ta++ )
		declareTypeAlias( pd, this, ta->name, ta->typeRef );

	/* Go into child aliases. */
	for ( NamespaceVect::Iter c = childNamespaces; c.lte(); c++ )
		(*c)->declare( pd );
}

void Compiler::setPrecedence()
{
	for ( PredDeclList::Iter predDecl = predDeclList; predDecl != 0; predDecl++ ) {
		predDecl->typeRef->lookupType( this );

		LangEl *langEl = predDecl->typeRef->uniqueType->langEl;
		langEl->predType = predDecl->predType;
		langEl->predValue = predDecl->predValue;
	}
}

/*
 * Type Declaration Root.
 */
void Compiler::typeDeclaration()
{
	makeIgnoreCollectors();

	rootNamespace->declare( this );

	/* Fill any empty scanners with a default token. */
	initEmptyScanners();

	/* Create the default scanner which will return single characters for us
	 * when we have no other scanner */
	createDefaultScanner();

	setPrecedence();
}
