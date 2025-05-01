#ifndef QT_VERSION_CHECK_H
#define QT_VERSION_CHECK_H

#include <QtGlobal>

// Check for minimum required Qt version
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
#error "Foccuss requires Qt 6.2.0 or newer"
#endif

#endif // QT_VERSION_CHECK_H 