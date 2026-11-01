#include "DIAdNetwork.h"
#include "Async/Async.h"
#include "Misc/ConfigCacheIni.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
	/** Nothing to show (shipping builds without an ad SDK). */
	class FDINoAdNetwork final : public IDIAdNetwork
	{
	public:
		virtual void Gather(TFunction<void(FDIAdConsent)> Done) override { Done({}); }
		virtual void ShowPrivacyOptions(TFunction<void(FDIAdConsent)> Done) override { Done({}); }
		virtual void ShowBanner() override {}
		virtual void HideBanner() override {}
	};

#if !UE_BUILD_SHIPPING
	/** Desktop development: consent is "given" at once and the HUD draws a placeholder bar where the banner goes. */
	class FDIPreviewAdNetwork final : public IDIAdNetwork
	{
	public:
		virtual void Gather(TFunction<void(FDIAdConsent)> Done) override { Done({ true, true }); }
		virtual void ShowPrivacyOptions(TFunction<void(FDIAdConsent)> Done) override { Done({ true, true }); }
		virtual void ShowBanner() override {}
		virtual void HideBanner() override {}
		virtual bool IsPreview() const override { return true; }
	};
#endif

#if PLATFORM_ANDROID
	/** Waiting for the Java consent flow; it answers through nativeDIAdsConsent on the UI thread. */
	TFunction<void(FDIAdConsent)> GPendingConsent;

	void CallJava(const char* Name, const char* Signature, const FString* Arg = nullptr)
	{
		JNIEnv* Env = FAndroidApplication::GetJavaEnv();
		if (!Env)
		{
			return;
		}
		jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, Name, Signature, false);
		if (!Method)
		{
			return;
		}
		if (Arg)
		{
			auto JavaArg = FJavaHelper::ToJavaString(Env, *Arg);
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, *JavaArg);
		}
		else
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
		}
	}

	/** Google Mobile Ads + UMP, through the Java added by DoubleIt_UPL.xml. Ids: [DoubleIt.Ads] in DefaultEngine.ini. */
	class FDIAdMobNetwork final : public IDIAdNetwork
	{
	public:
		virtual void Gather(TFunction<void(FDIAdConsent)> Done) override
		{
			GPendingConsent = MoveTemp(Done);
			CallJava("AndroidThunkJava_DIAdsGather", "()V");
		}
		virtual void ShowPrivacyOptions(TFunction<void(FDIAdConsent)> Done) override
		{
			GPendingConsent = MoveTemp(Done);
			CallJava("AndroidThunkJava_DIAdsPrivacyOptions", "()V");
		}
		virtual void ShowBanner() override
		{
			FString UnitId;
			GConfig->GetString(TEXT("DoubleIt.Ads"), TEXT("BannerUnitId"), UnitId, GEngineIni);
			CallJava("AndroidThunkJava_DIAdsShowBanner", "(Ljava/lang/String;)V", &UnitId);
		}
		virtual void HideBanner() override { CallJava("AndroidThunkJava_DIAdsHideBanner", "()V"); }
	};
#endif
}

#if PLATFORM_ANDROID
JNI_METHOD void Java_com_epicgames_unreal_GameActivity_nativeDIAdsConsent(JNIEnv* Env, jobject Thiz, jboolean bCanRequest, jboolean bPrivacyRequired)
{
	const FDIAdConsent Consent{ bCanRequest == JNI_TRUE, bPrivacyRequired == JNI_TRUE };
	AsyncTask(ENamedThreads::GameThread, [Consent]()
	{
		if (TFunction<void(FDIAdConsent)> Done = MoveTemp(GPendingConsent))
		{
			Done(Consent);
		}
	});
}
#endif

TSharedRef<IDIAdNetwork> DIAdNetworks::Make()
{
#if PLATFORM_ANDROID
	return MakeShared<FDIAdMobNetwork>();
#elif !UE_BUILD_SHIPPING
	return MakeShared<FDIPreviewAdNetwork>();
#else
	return MakeShared<FDINoAdNetwork>();
#endif
}
