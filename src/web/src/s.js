function G(id) {
	return document.getElementById(id);
}

var vector = {
	move: {
		x: 0.0,
		y: 0.0,
		z: 0.0,
	},
	rotate: {
		pitch: 0.0,
		roll:  0.0,
		yaw:   0.0,
	},
};

var telemetry = {
	status:   {},
	voltage:  0.0,
	current:  0.0,
	loopTime: 0,
};

var gui ={
	updateInterval: null,
	obj: {
		status: null,
		body_rotate: null,
	},

	init: function () {
		document.addEventListener("visibilitychange", gui.onVisibilityChange);
		gui.obj.status = G('status');
		gui.obj.body_rotate = G('body_rotate');

		gui.updateInterval = setInterval(gui.update, 100);
	},

	update: function () {
		gui.updateStatus();
	},

	onVisibilityChange: function () {
		if (document.visibilityState == "hidden") {
			// If browser closed
			failsafe.setFS();
		}
	},

	displayNumber: function (n) {
		if (n == 0) {
			return "0.000";
		}

		return (n + "000000").slice(0, 5);
	},

	updateStatus: function (){
		gui.obj.status.innerHTML = gui.displayNumber(telemetry.voltage) + 'V | ' + gui.displayNumber(telemetry.current) + 'A | LoopTime: ' + telemetry.loopTime;
	},
};

let ws = {
	ws:                null,
	status:            false,
	error:             false,
	updateInterval:    null,
	telemetryInterval: null,
	
	init: function () {
		clearInterval(ws.updateInterval);
		try {
			ws.ws = new WebSocket(c.w);
			ws.ws.onopen = function() {
				ws.status = true;
			};
			ws.ws.onerror = function() {
				ws.status = false;
			};
			ws.ws.binaryType = 'arraybuffer';
			ws.ws.onmessage = function (event) {
				ws.parseEvent(new Uint8Array(event.data));
			};

			ws.updateInterval    = setInterval(ws.update, 50);
			ws.telemetryInterval = setInterval(ws.telemetryRequest, 1000);
		} catch(e) {
			clearInterval(ws.updateInterval);
			ws.status = false;
			ws.error = true;
			console.log(e);
		};
	},

	update: function(data) {
		if (ws.status && (!window.calibControls || !window.calibControls.isCalibrating)) {
			ws.ws.send(packet.move());
		}
	},

	parseEvent: function (binaryData) {
		switch (binaryData[0]) {
			case 84:
				ws.telemetryResponse(binaryData);
				break;
		}
	},

	telemetryRequest: function (data) {
		if (ws.status) {
			ws.ws.send(packet.telemetry());
		}
	},

	telemetryResponse: function (binaryData) {
		packet.telemetryParse(binaryData);
	}
};

let failsafe = {
	setFS: function () {
		vector.move.x       = 0;
		vector.move.y       = 0;
		vector.move.z       = 0;
		vector.rotate.roll  = 0;
		vector.rotate.pitch = 0;
		vector.rotate.yaw   = 0;
	}
};


let packet = {
	init: function () {
		packet.pMove = new ArrayBuffer(14);
		packet.vMove = new Uint8Array(packet.pMove);
	},
	_norm1: function (value) {
		return (value+1)*10000;
	},
	_uint16: function (view, num, offset) {
		view[offset]   = (num>>8)&255;
		view[offset+1] = num&255;
	},
	_getUint16: function (data, offset) {
		return parseInt(data[offset]<<8 | data[offset+1]);
	},
	move: function () {
		packet.vMove[0] = 77;
		packet.vMove[1] = 1;
		packet._uint16(packet.vMove, packet._norm1(vector.move.x),        2);
		packet._uint16(packet.vMove, packet._norm1(vector.move.y),        4);
		packet._uint16(packet.vMove, packet._norm1(vector.move.z),        6);
		packet._uint16(packet.vMove, packet._norm1(vector.rotate.pitch),  8);
		packet._uint16(packet.vMove, packet._norm1(vector.rotate.roll),  10);
		packet._uint16(packet.vMove, packet._norm1(vector.rotate.yaw),   12);

		return packet.pMove;
	},
	telemetry: function () {
		packet.vMove[0] = 84;
		packet.vMove[1] = 1;

		return packet.pMove;
	},
	telemetryParse: function (binaryTelemetry) {
		telemetry.voltage  = packet._getUint16(binaryTelemetry, 4)/1000;
		telemetry.current  = packet._getUint16(binaryTelemetry, 6)/1000;
		telemetry.loopTime = packet._getUint16(binaryTelemetry, 8);
	}
}

class onScreenGamepad {
	constructor(obj, deadband, callback) {
		this.obj = obj,
		this.deadband = deadband || 0.05,
		this.callback = callback;
		
		this.isEvent = false;
		this.vector = {
				x: 0.0,
				y: 0.0,
			};
	}
	
	init () {
		this.obj.addEventListener('mousedown',   (event) => this.eventStart(event));
		this.obj.addEventListener('touchstart',  (event) => this.eventStart(event));
		
		this.obj.addEventListener('mouseup',     (event) => this.eventFinish(event));
		this.obj.addEventListener('mouseout',    (event) => this.eventFinish(event));
		this.obj.addEventListener('mouseleave',  (event) => this.eventFinish(event));
		this.obj.addEventListener('touchend',    (event) => this.eventFinish(event));
		this.obj.addEventListener('touchcancel', (event) => this.eventFinish(event));
		
		this.obj.addEventListener('mousemove',   (event) => this.eventMove(false, event));
		this.obj.addEventListener('touchmove',   (event) => this.eventMove(true, event));
	}
	
	eventStart() {
		this.isEvent = true;
		this.obj.classList.add('active');
	}
	
	eventFinish() {
		this.isEvent = false;
		this.vector.x = 0.0;
		this.vector.y = 0.0;
		this.display(0, 0);
		this.obj.classList.remove('active');
		
		this.callback(this.vector);
	}
	
	eventMove(isTouch, event) {
		let sendEvent = false;
		let x = ((isTouch ? event.targetTouches[0].clientX : event.clientX) - event.target.offsetLeft) / this.obj.offsetWidth*2-1;
		let y = ((isTouch ? event.targetTouches[0].clientY : event.clientY) - event.target.offsetTop) / this.obj.offsetHeight*2-1;
		if (Math.abs(x) <= this.deadband) x = 0;
		if (Math.abs(y) <= this.deadband) y = 0;
		if (x > 1) x = 1;
		if (x < -1) x =-1;
		if (y > 1) y = 1;
		if (y < -1) y =-1;
		if (!this.isEvent) {
			x = 0;
			y = 0;
		}
		if (this.vector.x !== x || this.vector.y !== y) {
			sendEvent = true;
		}
		this.vector.x =  x;
		this.vector.y = -y;
		
		if (sendEvent) {
			this.display(x, y);
			this.callback(this.vector);
		}
	}
	
	display(x, y) {
		this.obj.style.backgroundPosition = (x+1)/2*100 + '% ' + (y+1)/2*100 + '%';
	}
};

let control = {
	init() {
		let leftJ  = new onScreenGamepad(G('leftJ'),  0.05, this.leftJcallback);
		let rightJ = new onScreenGamepad(G('rightJ'), 0.05, this.rightJcallback);
		leftJ.init();
		rightJ.init();
	},
	leftJcallback(v) {
		vector.rotate.yaw = v.x;
	},
	rightJcallback(v) {
		/**
		 * Body rotation, this is temporary to easy validate IK
		 */
		if (gui.obj.body_rotate.checked) {
			vector.rotate.roll  = v.y;
			vector.rotate.pitch = v.x;
			
			return;
		}
		vector.move.x = v.x;
		vector.move.y = v.y;
	}
}

let gaitControls = {
	activeBtn: null,
	init() {
		const btns = {
			'btn_walk': { move: { x: 0, y: 0.5, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0 } },
			'btn_run': { move: { x: 0, y: 1.0, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0 } },
			'btn_sit': { move: { x: 0, y: 0, z: -0.8 }, rotate: { pitch: 0.2, roll: 0, yaw: 0 } },
			'btn_dance': { move: { x: 0, y: 0, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0.5 } }, // Basic dance rotation
			'btn_walk_back': { move: { x: 0, y: -0.5, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0 } },
			'btn_step_right': { move: { x: 0.5, y: 0, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0 } },
			'btn_step_left': { move: { x: -0.5, y: 0, z: 0 }, rotate: { pitch: 0, roll: 0, yaw: 0 } }
		};

		for (const [id, params] of Object.entries(btns)) {
			let el = G(id);
			if (!el) continue;
			el.addEventListener('click', () => {
				if (this.activeBtn === el) {
					// Turn off
					this.activeBtn.classList.remove('active');
					this.activeBtn = null;
					failsafe.setFS(); // Resets all vectors to 0
				} else {
					// Turn on new button
					if (this.activeBtn) this.activeBtn.classList.remove('active');
					this.activeBtn = el;
					this.activeBtn.classList.add('active');
					
					// Apply vectors
					vector.move.x = params.move.x;
					vector.move.y = params.move.y;
					vector.move.z = params.move.z;
					vector.rotate.pitch = params.rotate.pitch;
					vector.rotate.roll = params.rotate.roll;
					vector.rotate.yaw = params.rotate.yaw;
				}
			});
		}
	}
};

window.calibControls = {
	isCalibrating: false,
	init() {
		const cb = G('cb_calibrate');
		const panel = G('calibration-panel');
		const slidersContainer = G('calib-sliders');
        const gaitContainer = G('gait-controls');
        const ctrlContainer = G('control');

		const legNames = ["Front Left", "Front Right", "Hind Left", "Hind Right"];
		const jointNames = ["Alpha (Hip)", "Beta (Femur)", "Gamma (Tibia)"];

		// Build sliders
		for (let leg = 0; leg < 4; leg++) {
			let legDiv = document.createElement('div');
			legDiv.innerHTML = `<strong style="font-size:0.9em; display:block; margin-bottom:5px;">${legNames[leg]}</strong>`;
			for (let joint = 0; joint < 3; joint++) {
				let wrapper = document.createElement('div');
				wrapper.innerHTML = `<label style="font-size:0.8em; display:inline-block; width:80px;">${jointNames[joint]}</label>
									 <button class="calib-btn" id="calib_dec_${leg}_${joint}">-</button>
									 <input type="range" min="-25" max="25" step="0.5" value="0" id="calib_${leg}_${joint}" style="width:calc(100% - 170px); vertical-align:middle;">
									 <button class="calib-btn" id="calib_inc_${leg}_${joint}">+</button>
									 <span id="calib_val_${leg}_${joint}" style="font-size:0.8em; display:inline-block; width:40px; text-align:right;">0.0&deg;</span>`;
				legDiv.appendChild(wrapper);

				setTimeout(() => {
					let slider = G(`calib_${leg}_${joint}`);
					let valDisplay = G(`calib_val_${leg}_${joint}`);
					let btnDec = G(`calib_dec_${leg}_${joint}`);
					let btnInc = G(`calib_inc_${leg}_${joint}`);

					let lastFetchTime = 0;
					const updateVal = (val) => {
						slider.value = val;
						valDisplay.innerText = val + '°';
						let now = Date.now();
						if (now - lastFetchTime > 50) { // max 20Hz
							lastFetchTime = now;
							fetch(`/calib?action=set&leg=${leg}&joint=${joint}&val=${val}`);
						}
					};

					slider.addEventListener('input', (e) => {
						updateVal(e.target.value);
					});
					slider.addEventListener('change', (e) => {
						fetch(`/calib?action=set&leg=${leg}&joint=${joint}&val=${e.target.value}`);
					});

					btnDec.addEventListener('click', () => {
						let v = parseFloat(slider.value) - 0.5;
						updateVal(v);
						fetch(`/calib?action=set&leg=${leg}&joint=${joint}&val=${v}`);
					});
					btnInc.addEventListener('click', () => {
						let v = parseFloat(slider.value) + 0.5;
						updateVal(v);
						fetch(`/calib?action=set&leg=${leg}&joint=${joint}&val=${v}`);
					});
				}, 100);
			}
			slidersContainer.appendChild(legDiv);
		}

		const pwmMin = G('calib_pwm_min');
		const pwmMax = G('calib_pwm_max');
		const maxAngle = G('calib_max_angle');

		const updatePwm = () => {
			fetch(`/calib?action=pwm&min=${pwmMin.value}&max=${pwmMax.value}&angle=${maxAngle.value}`);
		};
		pwmMin.addEventListener('change', updatePwm);
		pwmMax.addEventListener('change', updatePwm);
		maxAngle.addEventListener('change', updatePwm);

		cb.addEventListener('change', (e) => {
			this.isCalibrating = e.target.checked;
			if (this.isCalibrating) {
				panel.style.display = 'flex';
                gaitContainer.style.display = 'none';
                ctrlContainer.style.display = 'none';
				
				// Fetch current values
				fetch('/calib?action=get').then(r => r.json()).then(data => {
					if (data.pwmMin !== undefined) pwmMin.value = data.pwmMin;
					if (data.pwmMax !== undefined) pwmMax.value = data.pwmMax;
					if (data.maxAngle !== undefined) maxAngle.value = data.maxAngle;
					for (let leg = 0; leg < 4; leg++) {
						for (let joint = 0; joint < 3; joint++) {
							let slider = G(`calib_${leg}_${joint}`);
							let valDisplay = G(`calib_val_${leg}_${joint}`);
							if (slider && data[leg]) {
								slider.value = data[leg][joint];
								valDisplay.innerText = data[leg][joint] + '°';
							}
						}
					}
					fetch('/calib?action=start');
				});
			} else {
				panel.style.display = 'none';
                gaitContainer.style.display = 'flex';
                ctrlContainer.style.display = 'block';
				fetch('/calib?action=stop');
			}
		});
	}
};

control.init();
gui.init();
packet.init();
ws.init();
gaitControls.init();
window.calibControls.init();

