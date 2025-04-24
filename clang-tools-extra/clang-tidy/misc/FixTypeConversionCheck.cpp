//===--- FixStdstringDataAccessCheck.cpp - clang-tidy ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FixTypeConversionCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

#include <iostream>




using namespace clang::ast_matchers;

namespace clang::tidy::misc {

void FixTypeConversionCheck::registerMatchers(MatchFinder *Finder) {

  const auto rnStringTypeMatch = qualType(anyOf(asString("RNString"),
                                                  asString("const RNString")));


  const auto cStringTypeMatch = qualType(anyOf(asString("CString"),
                                                  asString("const CString")));


  Finder->addMatcher(
      declStmt(unless(isExpansionInSystemHeader()),
               has(varDecl(
                   hasType(cStringTypeMatch),
                   hasDescendant(
                       cxxBindTemporaryExpr(
                           hasType(rnStringTypeMatch),
                           unless(hasAncestor(cxxMemberCallExpr(
                               callee(cxxMethodDecl(hasName("GetString"))))))

                               )
                           .bind("cxxBindTemporaryExpr"))))

                   )
          .bind("declStmt"),
      this);
  Finder->addMatcher(
      declStmt(
          unless(isExpansionInSystemHeader()),
          has(varDecl(hasType(rnStringTypeMatch),
                      anyOf(hasDescendant(declRefExpr(hasType(cStringTypeMatch))
                                              .bind("declRefExpr")),
                            hasDescendant(memberExpr(hasType(cStringTypeMatch))
                                              .bind("memberExpr"))))))
          .bind("declStmt"),
      this);
}

void FixTypeConversionCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *Call =
      Result.Nodes.getNodeAs<CXXBindTemporaryExpr>("cxxBindTemporaryExpr");

  if (Call) {

    diag(Call->getEndLoc(),
         "Consider adding .GetString() to the CString object")
        << FixItHint::CreateInsertion(Call->getEndLoc().getLocWithOffset(1),
                                        ".GetString()");
  }


  const auto processExpr = [&](auto* expr){
    if(!expr)
      return;

    const SourceManager &SM = *Result.SourceManager;
    const LangOptions &LangOpts = Result.Context->getLangOpts();

    SourceLocation EndLoc =
        Lexer::getLocForEndOfToken(expr->getEndLoc(), 0, SM, LangOpts);

    diag(expr->getEndLoc(), "Consider adding .GetString() call")
        << FixItHint::CreateInsertion(EndLoc, ".GetString()");
  };

  const auto *declRefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr");
  processExpr(declRefExpr);

  const auto *memberExpr = Result.Nodes.getNodeAs<MemberExpr>("memberExpr");
  processExpr(memberExpr);
}

} // namespace clang::tidy::misc
