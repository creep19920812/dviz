/****************************************************************************
** Meta object code from reading C++ file 'DocumentSettingsDialog.h'
**
** Created: Sat Dec 19 21:08:45 2009
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../DocumentSettingsDialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DocumentSettingsDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DocumentSettingsDialog[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // slots: signature, parameters, type, tag, flags
      24,   23,   23,   23, 0x09,
      35,   23,   23,   23, 0x09,
      44,   23,   23,   23, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_DocumentSettingsDialog[] = {
    "DocumentSettingsDialog\0\0accepted()\0"
    "set4x3()\0set16x9()\0"
};

const QMetaObject DocumentSettingsDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_DocumentSettingsDialog,
      qt_meta_data_DocumentSettingsDialog, 0 }
};

const QMetaObject *DocumentSettingsDialog::metaObject() const
{
    return &staticMetaObject;
}

void *DocumentSettingsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DocumentSettingsDialog))
        return static_cast<void*>(const_cast< DocumentSettingsDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int DocumentSettingsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: accepted(); break;
        case 1: set4x3(); break;
        case 2: set16x9(); break;
        default: ;
        }
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
