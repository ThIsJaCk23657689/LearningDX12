#pragma once
#include <cmath>
#include <cstdint>
#include <exception>
#include <Windows.h>

// copy from DirectXTK12 StepTimer

class StepTimer
{
public:
	StepTimer() noexcept( false ) :
		m_elapsedTicks( 0 ),
		m_totalTicks( 0 ),
		m_leftOverTicks( 0 ),
		m_frameCount( 0 ),
		m_framePerSecond( 0 ),
		m_frameThisSecond( 0 ),
		m_qpcSecondCounter( 0 ),
		m_isFixedTimeStep( false ),
		m_targetElapsedTicks( uTicksPerSecond / 60 )
	{
		if ( !QueryPerformanceFrequency( &m_qpcFrequency ) )
		{
			throw std::exception();
		}

		if ( !QueryPerformanceCounter( &m_qpcLastTime ) )
		{
			throw std::exception();
		}

		// Initialize max delta to 1/10 of a second.
		m_qpcMaxDelta = static_cast< uint64_t >( m_qpcFrequency.QuadPart / 10 );
	}

	// Get elapsed time since the previous Update call.
	uint64_t GetElapsedTicks() const noexcept { return m_elapsedTicks; }
	double GetElapsedSeconds() const noexcept { return TicksToSeconds( m_elapsedTicks ); }

	// Get total time since the start of the program.
	uint64_t GetTotalTicks() const noexcept { return m_totalTicks; }
	double GetTotalSeconds() const noexcept { return TicksToSeconds( m_totalTicks ); }

	// Get total number of updates since start of the program.
	uint32_t GetFrameCount() const noexcept { return m_frameCount; }

	// Get the current framerate.
	uint32_t GetFramePerSecond() const noexcept { return m_framePerSecond; }

	// Set how often to call Update when in fixed timestep mode.
	void SetTargetElapsedTicks( uint64_t targetElapsed ) noexcept { m_targetElapsedTicks = targetElapsed; }
	void SetTargetElapsedSeconds( double targetElapsed ) noexcept { m_targetElapsedTicks = SecondsToTicks( targetElapsed ); }

	// Integer format represents time using 10,000,000 ticks per second.
	static constexpr uint64_t uTicksPerSecond = 10000000;

	static constexpr double TicksToSeconds( uint64_t ticks ) noexcept { return static_cast< double >( ticks ) / uTicksPerSecond; }
	static constexpr uint64_t SecondsToTicks( double seconds ) noexcept { return static_cast< uint64_t >( seconds * uTicksPerSecond ); }

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timstep logic attempt a set of catch-up Update calls.
	void ResetElapsedTime()
	{
		if ( !QueryPerformanceCounter( &m_qpcLastTime ) )
		{
			throw std::exception();
		}

		m_leftOverTicks = 0;
		m_framePerSecond = 0;
		m_frameThisSecond = 0;
		m_qpcSecondCounter = 0;
	}

	// Update timer state, calling the specified Update function the appropriate number of times.
	template< typename TUpdate >
	void Tick( const TUpdate& update )
	{
		// Query the current time.
		LARGE_INTEGER currentTime;
		if ( !QueryPerformanceCounter( &currentTime ) )
		{
			throw std::exception();
		}

		uint64_t timeDelta = static_cast< uint64_t >( currentTime.QuadPart - m_qpcLastTime.QuadPart );

		m_qpcLastTime = currentTime;
		m_qpcSecondCounter += timeDelta;

		// Clamp excessively large time deltas (e.g. after pause in the debugger).
		if ( timeDelta > m_qpcMaxDelta )
		{
			timeDelta = m_qpcMaxDelta;
		}

		// Convert QPC uints into a canonical tick foramt. This cannot overflow due to the previous clamp.
		timeDelta *= uTicksPerSecond;
		timeDelta /= static_cast< uint64_t >( m_qpcFrequency.QuadPart );

		const uint32_t lastFrameCount = m_frameCount;
		if ( m_isFixedTimeStep )
		{
			// Fixed timestep update logic

			// If the app is running very close to the target elpased time (when 1/4 of millisecond) just clamp
			// the clock to exactly match the target value. This prevents tiny and irrelevant errors
			// from accumulating over time. Without this clamping, a game that requested a 60 FPS fixed update,
			// running with vsync enabled on a 59.94 NTSC display, would eventually accumulate enough tiny errors
			// that it woud drop a frame. 
			// It is better to just round small deviations down to zero to leave things running smoothly.

			if ( static_cast< uint64_t >( std::abs( static_cast< int64_t >( timeDelta - m_targetElapsedTicks ) ) ) < uTicksPerSecond / 4000 )
			{
				timeDelta = m_targetElapsedTicks;
			}

			m_leftOverTicks += timeDelta;

			while ( m_leftOverTicks >= m_targetElapsedTicks )
			{
				m_elapsedTicks = m_targetElapsedTicks;
				m_totalTicks += m_targetElapsedTicks;
				m_leftOverTicks -= m_targetElapsedTicks;
				m_frameCount++;

				update();
			}
		}
		else
		{
			// Variable timestep update logic.
			m_elapsedTicks = timeDelta;
			m_totalTicks += timeDelta;
			m_leftOverTicks = 0;
			m_frameCount++;

			update();
		}

		// Track the current framerate.
		if ( m_frameCount != lastFrameCount )
		{
			m_frameThisSecond++;
		}

		if ( m_qpcSecondCounter >= static_cast< uint64_t >( m_qpcFrequency.QuadPart ) )
		{
			m_framePerSecond = m_frameThisSecond;
			m_frameThisSecond = 0;
			m_qpcSecondCounter %= static_cast< uint64_t >( m_qpcFrequency.QuadPart );
		}
	}

private:
	// Source timing data use QPC units.
	LARGE_INTEGER m_qpcFrequency;
	LARGE_INTEGER m_qpcLastTime;
	uint64_t m_qpcMaxDelta;

	// Derived timing data use a canonical tick format.
	uint64_t m_elapsedTicks;
	uint64_t m_totalTicks;
	uint64_t m_leftOverTicks;

	// Memebers for tracking the framerate.
	uint32_t m_frameCount;
	uint32_t m_framePerSecond;
	uint32_t m_frameThisSecond;
	uint64_t m_qpcSecondCounter;

	// Member for configuring fixed timestep mode.
	bool m_isFixedTimeStep;
	uint64_t m_targetElapsedTicks;
};
