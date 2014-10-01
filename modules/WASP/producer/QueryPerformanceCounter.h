class CStopwatch {
public:
   CStopwatch() { QueryPerformanceFrequency(&m_liPerfFreq); Start(); }

   void Start() { QueryPerformanceCounter(&m_liPerfStart); }

   double Now() const {   // Returns # of milliseconds since Start was called
      LARGE_INTEGER liPerfNow;
      QueryPerformanceCounter(&liPerfNow);
      return(( (double)(liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000) 
         / (double)m_liPerfFreq.QuadPart);
   }

private:
   LARGE_INTEGER m_liPerfFreq;   // Counts per second
   LARGE_INTEGER m_liPerfStart;  // Starting count
};