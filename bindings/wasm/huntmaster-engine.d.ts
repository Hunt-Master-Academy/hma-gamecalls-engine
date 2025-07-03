// File: huntmaster-engine.d.ts
declare module "huntmaster-engine" {
    export interface ProcessingResult {
        success: boolean;
        score?: number;
        framesProcessed?: number;
        processingTimeMs?: number;
        error?: string;
    }
    
    export interface AudioChunk {
        frameIndex: number;
        energyLevel: number;
        containsVoice: boolean;
        samples: number;
    }
    
    export interface ProcessorStats {
        chunksProcessed: number;
        chunksDropped: number;
        bufferOverruns: number;
        bufferUnderruns: number;
        avgLatencyMs: number;
        currentBufferUsage: number;
    }
    
    export interface PerformanceStats {
        processor: ProcessorStats;
        memoryUsageMB: number;
        activeSessionCount: number;
        uptimeSeconds: number;
    }
    
    export interface WorkerStatus {
        initialized: boolean;
        processing: boolean;
        bufferSize: number;
    }
    
    export class HuntmasterEngine {
        constructor();
        
        initialize(sampleRate: number, frameSize: number, mfccCoeffs: number): boolean;
        shutdown(): void;
        isInitialized(): boolean;
        
        loadMasterCall(callName: string, audioData: Float32Array): boolean;
        
        processAudioChunk(audioPtr: number, numSamples: number): number;
        processAudioArray(audioArray: Float32Array): ProcessingResult;
        
        startSession(): number;
        endSession(sessionId: number): boolean;
        getActiveSessionCount(): number;
        
        enableStreaming(enable: boolean): boolean;
        enqueueAudioBuffer(buffer: Float32Array): boolean;
        dequeueResults(): AudioChunk[];
        
        getPerformanceStats(): PerformanceStats;
        resetStats(): void;
        
        onMemoryPressure(): void;
        getMemoryUsage(): number;
    }
    
    export class HuntmasterAudioWorker {
        constructor();
        
        initialize(sharedBufferPtr: number, bufferSize: number): boolean;
        processSharedBuffer(): void;
        getStatus(): WorkerStatus;
    }
}