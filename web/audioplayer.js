class AudioPlayer {
	constructor(audioContext, rate){
		let sources = [];
		let samples = 0;
		const baseTime = audioContext.currentTime;

		this.frames = rate / 100;
		this.paused = true;
		this._shutdown = false;
		this.onbuffer = ()=>{};

		const read = ()=>{
			const audioBuffer = audioContext.createBuffer(2, this.frames, rate);

			if(!this.paused){
				this.onbuffer(audioBuffer, frames);
			}

			const currentSource = audioContext.createBufferSource();
			currentSource.buffer = audioBuffer;
			currentSource.connect(audioContext.destination);

			currentSource.onended = function() {
				sources = sources.filter((x)=>x != currentSource);

				if(this._shutdown) return;

				for(let i = sources.length; i < 5; i++) read();
			}

			sources.push(currentSource);

			currentSource.start(baseTime + samples / rate);

			samples += frames;
		}

		read();
		read();
		read();
		read();
		read();
	}

	resume(){
		this.paused = false;
	}

	pause(){
		this.paused = true;
	}

	shutdown(){
		this.shutdown = true;
	}
};
