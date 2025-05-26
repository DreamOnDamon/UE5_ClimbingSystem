#pragma once

namespace Debug
{
    /**
     * Prints a debug message to screen if GEngine is exist, but print to log 
     * output anyway
     * 
     * @param Message    The text to display
     * @param Color      Display color (defaults to random color)
     * @param Key        Unique key to prevent message dep;ication (-1 = transient)
    */
    static void Print(
        const FString& Message,
        const FColor& Color = FColor::MakeRandomColor(),
        int32 InKey = -1
    )
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(InKey, 6.f, Color, Message);
        }

        UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
    }
}