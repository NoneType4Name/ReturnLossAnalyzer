#include "visaResourceManager.hpp"
#include <QDebug>

VisaResourceManager &VisaResourceManager::instance()
{
    static VisaResourceManager s_instance;
    return s_instance;
}

VisaResourceManager::VisaResourceManager()
{
    ViStatus st = viOpenDefaultRM( &m_rm );
    if ( st < VI_SUCCESS )
    {
        m_lastError = "viOpenDefaultRM failed: " + statusToString( st );
        m_rm        = VI_NULL;
        qWarning() << m_lastError;
    }
}

VisaResourceManager::~VisaResourceManager()
{
    if ( m_rm != VI_NULL )
    {
        viClose( m_rm );
        m_rm = VI_NULL;
    }
}

QString VisaResourceManager::statusToString( ViStatus st )
{
    char buf[ 256 ] = { 0 };
    viStatusDesc( VI_NULL, st, buf );
    return QString::fromLatin1( buf );
}

QVector<QString> VisaResourceManager::listResources() const
{
    QVector<QString> result;
    if ( m_rm == VI_NULL ) return result;

    QMutexLocker lock { &m_mutex };

    ViFindList findList           = VI_NULL;
    ViUInt32 count                = 0;
    ViChar desc[ VI_FIND_BUFLEN ] = { 0 };

    ViStatus st = viFindRsrc( m_rm, "USB?*INSTR", &findList, &count, desc );
    if ( st < VI_SUCCESS )
    {
        m_lastError = "viFindRsrc failed: " + statusToString( st );
        return result;
    }

    for ( ViUInt32 i = 0; i < count; ++i )
    {
        result.append( QString::fromLatin1( desc ) );
        if ( i + 1 < count )
            viFindNext( findList, desc );
    }
    viClose( findList );
    return result;
}

ViSession VisaResourceManager::openSession( const QString &resource, int timeoutMs )
{
    if ( m_rm == VI_NULL )
    {
        m_lastError = "Resource manager not initialized";
        return VI_NULL;
    }
    ViSession instr = VI_NULL;
    ViStatus st     = viOpen( m_rm,
                              resource.toLocal8Bit().constData(),
                              VI_NULL, VI_NULL, &instr );
    if ( st < VI_SUCCESS )
    {
        m_lastError = "viOpen failed: " + statusToString( st );
        return VI_NULL;
    }
    viSetAttribute( instr, VI_ATTR_TMO_VALUE, timeoutMs );
    return instr;
}

void VisaResourceManager::closeSession( ViSession &session )
{
    if ( session != VI_NULL )
    {
        viClose( session );
        session = VI_NULL;
    }
}

QString VisaResourceManager::lastError() const
{
    return m_lastError;
}