
class CCriticSection
{
private:

	CRITICAL_SECTION m_CriticSec;

public:

	HRESULT Init();
	HRESULT UnInit();
	HRESULT Lock();
	HRESULT Unlock();
};