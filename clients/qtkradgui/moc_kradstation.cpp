/****************************************************************************
** Meta object code from reading C++ file 'kradstation.h'
**
** Created: Sun Jan 20 21:55:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "kradstation.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kradstation.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_KradStation[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   13,   12,   12, 0x05,
      73,   61,   12,   12, 0x05,

 // slots: signature, parameters, type, tag, flags
     121,   13,   12,   12, 0x0a,
     171,  165,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_KradStation[] = {
    "KradStation\0\0portgroup\0"
    "portgroupAdded(kr_mixer_portgroup_t*)\0"
    "control_rep\0volumeUpdate(kr_mixer_portgroup_control_rep_t*)\0"
    "handlePortgroupAdded(kr_mixer_portgroup_t*)\0"
    "value\0setVolume(int)\0"
};

void KradStation::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        KradStation *_t = static_cast<KradStation *>(_o);
        switch (_id) {
        case 0: _t->portgroupAdded((*reinterpret_cast< kr_mixer_portgroup_t*(*)>(_a[1]))); break;
        case 1: _t->volumeUpdate((*reinterpret_cast< kr_mixer_portgroup_control_rep_t*(*)>(_a[1]))); break;
        case 2: _t->handlePortgroupAdded((*reinterpret_cast< kr_mixer_portgroup_t*(*)>(_a[1]))); break;
        case 3: _t->setVolume((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData KradStation::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject KradStation::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_KradStation,
      qt_meta_data_KradStation, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &KradStation::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *KradStation::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *KradStation::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_KradStation))
        return static_cast<void*>(const_cast< KradStation*>(this));
    return QObject::qt_metacast(_clname);
}

int KradStation::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void KradStation::portgroupAdded(kr_mixer_portgroup_t * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void KradStation::volumeUpdate(kr_mixer_portgroup_control_rep_t * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
