#ifndef KGAMESVGDOCUMENTTEST_H
#define KGAMESVGDOCUMENTTEST_H

#include "qtest_kde.h"
#include <kgamesvgdocument.h>

class tst_KGameSvgDocument : public QObject
{
    Q_OBJECT

    KGameSvgDocument m_svgDom;

// Declare test functions as private slots, or they won't get executed
private slots:

    /// @brief This function is called first, so you can do init stuff here.
    void initTestCase();

    /// @brief Test various style manipulations to make sure they are reversible
    void style();

    /// @brief We scale up, then back, and verify scaling is reversible
    void scale();

    /// @brief Test the transform attribute QRegExp
    void transformRegex();

    /// @brief We test that transforms can be read and written to DOM
    void transform();

    /// @brief This function is called last, so you can do final stuff here.
    void cleanupTestCase();
};

#endif // KGAMESVGDOCUMENTTEST_H
