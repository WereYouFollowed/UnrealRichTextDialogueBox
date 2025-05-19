// Copyright (c) Sam Bloomberg

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlock.h"
#include "Framework/Text/RichTextLayoutMarshaller.h"
#include "Framework/Text/SlateTextLayout.h"
#include "DialogueBox.generated.h"

struct FDialogueTextSegment;

/**
 * A text block that exposes more information about text layout.
 */
UCLASS()
class SALCORE_GAME_API UDialogueTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	FORCEINLINE TSharedPtr<IRichTextMarkupParser> GetTextParser() const
	{
		return TextParser;
	}

	FORCEINLINE void ConfigureFromParent(const TArray<FDialogueTextSegment>* InSegments, const int32* InCurrentSegmentIndex)
	{
		Segments = InSegments;
		CurrentSegmentIndex = InCurrentSegmentIndex;
	}

	// variants to feed slate widget more info
	void SetTextPartiallyTyped(const FText& InText, const FText& InFinalText);
	void SetTextFullyTyped(const FText& InText);

protected:
	// implementation hidden in favour of explicit variants
	void SetText(const FText& InText) override
	{
		URichTextBlock::SetText(InText);
	}

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<IRichTextMarkupParser> TextParser;

	const TArray<FDialogueTextSegment>* Segments;
	const int32* CurrentSegmentIndex;
};

struct SALCORE_GAME_API FDialogueTextSegment
{
	FString Text;
	FTextRunParseResults RunInfo;
};

UCLASS()
class SALCORE_GAME_API UDialogueBox : public UUserWidget
{
	GENERATED_BODY()

public:
	UDialogueBox(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UDialogueTextBlock> LineText;

	// The amount of time between printing individual letters (for the "typewriter" effect).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Box")
	float LetterPlayTime = 0.025f;

	// The amount of time to wait after finishing the line before actually marking it completed.
	// This helps prevent accidentally progressing dialogue on short lines.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Box")
	float EndHoldTime = 0.15f;

	// Initialise future contents of dialogue box, but do not begin playing yet.
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void SetLine(const FText& InLine);
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void PlayLine(const FText& InLine);

	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void PlayToEnd();
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void PlayUntil(int32 idx);

	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void GetCurrentLine(FText& OutLine) const { OutLine = CurrentLine; }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	bool HasFinishedPlayingLine() const { return bHasFinishedPlaying; }
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	bool HasFinishedPlayingAnimation() const { return !LetterTimer.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void SkipToLineEnd();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueBoxOnPlayLetter);
	UPROPERTY(BlueprintAssignable, Category = "Dialogue Box")
	FDialogueBoxOnPlayLetter OnPlayLetter;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueBoxOnLineFinishedPlaying);
	UPROPERTY(BlueprintAssignable, Category = "Dialogue Box")
	FDialogueBoxOnLineFinishedPlaying OnLineFinishedPlaying;

protected:
	void NativeOnInitialized() override;

private:
	void PlayNextLetter();

	struct WrappedString
	{
		WrappedString(UDialogueTextBlock* LineText, const FText& CurrentLine);

		TArray<FDialogueTextSegment> Segments;
		int32 MaxLetterIndex;
	};
	class WrappedStringIterator
	{
	public:
		WrappedStringIterator(const WrappedString& parent)
		:
		m_parent(parent)
		{
		}

		void operator++()
		{
			CurrentLetterIndex++;
			CachedResultText = FText::FromString(evaluate());
		}
		const FText& get() const
		{
			return CachedResultText;
		}

		const int32& getCurrentSegmentIndex() const { return CurrentSegmentIndex; }
		void setCurrentLetterIndex(int32 idx) { CurrentLetterIndex = idx; CachedResultText = FText::FromString(evaluate()); }
		const int32& getCurrentLetterIndex () const { return CurrentLetterIndex;  }

	private:
		FString evaluate();

		// The section of the text that's already been printed out and won't ever change.
		// This lets us cache some of the work we've already done. We can't cache absolutely
		// everything as the last few characters of a string may change if they're related to
		// a named run that hasn't been completed yet.
		FString CachedSegmentText;

		FText CachedResultText;

		int32 CurrentSegmentIndex = 0;
		int32 CurrentLetterIndex = 0;

		const WrappedString& m_parent;
	};

	UPROPERTY()
	FText CurrentLine;

	TOptional<WrappedString> BuiltString;
	TOptional<WrappedStringIterator> BuiltStringIterator;

	int32 MaxLetterIndex = 0;

	uint32 bHasFinishedPlaying : 1;

	FTimerHandle LetterTimer;
};