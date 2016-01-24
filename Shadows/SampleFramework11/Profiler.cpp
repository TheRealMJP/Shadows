//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"
#include "Profiler.h"
#include "Utility.h"
#include "SpriteRenderer.h"
#include "SpriteFont.h"

using std::wstring;
using std::map;


namespace SampleFramework11
{

// == Profiler ====================================================================================

Profiler Profiler::GlobalProfiler;

void Profiler::Initialize(ID3D11Device* device, ID3D11DeviceContext* immContext)
{
    this->device = device;
    this->context = immContext;
}

void Profiler::StartProfile(const wstring& name)
{
    ProfileData& profileData = profiles[name];
    Assert_(profileData.QueryStarted == false);
    Assert_(profileData.QueryFinished == false);
    profileData.CPUProfile = false;
    profileData.Active = true;

    if(profileData.DisjointQuery[currFrame] == NULL)
    {
        // Create the queries
        D3D11_QUERY_DESC desc;
        desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        desc.MiscFlags = 0;
        DXCall(device->CreateQuery(&desc, &profileData.DisjointQuery[currFrame]));

        desc.Query = D3D11_QUERY_TIMESTAMP;
        DXCall(device->CreateQuery(&desc, &profileData.TimestampStartQuery[currFrame]));
        DXCall(device->CreateQuery(&desc, &profileData.TimestampEndQuery[currFrame]));
    }

    // Start a disjoint query first
    context->Begin(profileData.DisjointQuery[currFrame]);

    // Insert the start timestamp
    context->End(profileData.TimestampStartQuery[currFrame]);

    profileData.QueryStarted = true;
}

void Profiler::EndProfile(const wstring& name)
{
    ProfileData& profileData = profiles[name];
    Assert_(profileData.QueryStarted == true);
    Assert_(profileData.QueryFinished == false);

    // Insert the end timestamp
    context->End(profileData.TimestampEndQuery[currFrame]);

    // End the disjoint query
    context->End(profileData.DisjointQuery[currFrame]);

    profileData.QueryStarted = false;
    profileData.QueryFinished = true;
}

void Profiler::StartCPUProfile(const wstring& name)
{
    ProfileData& profileData = profiles[name];
    Assert_(profileData.QueryStarted == false);
    Assert_(profileData.QueryFinished == false);
    profileData.CPUProfile = true;
    profileData.Active = true;

    timer.Update();
    profileData.StartTime = timer.ElapsedMicroseconds();

    profileData.QueryStarted = true;
}

void Profiler::EndCPUProfile(const wstring& name)
{
    ProfileData& profileData = profiles[name];
    Assert_(profileData.QueryStarted == true);
    Assert_(profileData.QueryFinished == false);

    timer.Update();
    profileData.EndTime = timer.ElapsedMicroseconds();

    profileData.QueryStarted = false;
    profileData.QueryFinished = true;
}

void Profiler::EndFrame(SpriteRenderer& spriteRenderer, SpriteFont& spriteFont)
{
    // If any profile was previously active but wasn't used this frame, it could still
    // have outstanding queries that we need to keep running
    for(auto iter = profiles.begin(); iter != profiles.end(); iter++)
    {
        const std::wstring& name = (*iter).first;
        ProfileData& profile = (*iter).second;

        if(!profile.CPUProfile && !profile.Active && profile.DisjointQuery[0] != nullptr)
        {
            StartProfile(name);
            EndProfile(name);
            profile.Active = false;
        }
    }

    currFrame = (currFrame + 1) % QueryLatency;

    Float4x4 transform;
    transform.SetTranslation(Float3(25.0f, 100.0f, 0.0f));

    // Iterate over all of the profiles
    for(auto iter = profiles.begin(); iter != profiles.end(); iter++)
    {
        ProfileData& profile = (*iter).second;
        profile.QueryFinished = false;

        float time = 0.0f;
        if(profile.CPUProfile)
        {
            time = (profile.EndTime - profile.StartTime) / 1000.0f;
        }
        else
        {
            if(profile.DisjointQuery[currFrame] == NULL)
                continue;

            // Get the query data
            uint64 startTime = 0;
            while(context->GetData(profile.TimestampStartQuery[currFrame], &startTime, sizeof(startTime), 0) != S_OK);

            uint64 endTime = 0;
            while(context->GetData(profile.TimestampEndQuery[currFrame], &endTime, sizeof(endTime), 0) != S_OK);

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
            while(context->GetData(profile.DisjointQuery[currFrame], &disjointData, sizeof(disjointData), 0) != S_OK);

            if(disjointData.Disjoint == false)
            {
                uint64 delta = endTime - startTime;
                float frequency = static_cast<float>(disjointData.Frequency);
                time = (delta / frequency) * 1000.0f;
            }
        }

        profile.TimeSamples[profile.CurrSample] = time;
        profile.CurrSample = (profile.CurrSample + 1) % ProfileData::FilterSize;

        float sum = 0.0f;
        for(UINT i = 0; i < ProfileData::FilterSize; ++i)
            sum += profile.TimeSamples[i];
        time = sum / ProfileData::FilterSize;

        if(profile.Active)
        {
            wstring output = (*iter).first + L": " + ToString(time) + L"ms";
            spriteRenderer.RenderText(spriteFont, output.c_str(), transform, Float4(1.0f, 1.0f, 0.0f, 1.0f));
            transform._42 += 25.0f;
        }

        profile.Active = false;
    }
}

// == ProfileBlock ================================================================================

ProfileBlock::ProfileBlock(const std::wstring& name) : name(name)
{
    Profiler::GlobalProfiler.StartProfile(name);
}

ProfileBlock::~ProfileBlock()
{
    Profiler::GlobalProfiler.EndProfile(name);
}

// == CPUProfileBlock =============================================================================

CPUProfileBlock::CPUProfileBlock(const std::wstring& name) : name(name)
{
    Profiler::GlobalProfiler.StartCPUProfile(name);
}

CPUProfileBlock::~CPUProfileBlock()
{
    Profiler::GlobalProfiler.EndCPUProfile(name);
}

}