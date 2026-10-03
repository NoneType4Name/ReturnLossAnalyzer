#pragma once

#include <QString>
#include <QVector>
#include <QMutex>
#include <visa.h>

class VisaResourceManager
{
  public:
    static VisaResourceManager &instance();

    VisaResourceManager( const VisaResourceManager & )            = delete;
    VisaResourceManager &operator=( const VisaResourceManager & ) = delete;

    QVector<QString> listResources() const;

    ViSession openSession( const QString &resource, int timeoutMs );

    // Закрытие конкретной сессии
    void closeSession( ViSession &session );

    // Проверка "жив ли" RM
    bool isValid() const noexcept
    {
        return m_rm != VI_NULL;
    }

    QString lastError() const;

  private:
    VisaResourceManager();
    ~VisaResourceManager();

    static QString statusToString( ViStatus st );

    ViSession m_rm = VI_NULL;
    mutable QMutex m_mutex;
    mutable QString m_lastError;
};