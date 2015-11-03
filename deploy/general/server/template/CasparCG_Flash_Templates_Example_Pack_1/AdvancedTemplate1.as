/*
 * This template will count down from a specified amount of seconds in the MM:SS format. When it reaches 0 it will pause for 3 seconds and then remove itself. 
 * It takes the parameters "time",which is the time in seconds, and the parameter "image", that will load an image.
 * 
 * To compile this template you will need the free Tweener library found at http://code.google.com/p/tweener/
*/

package
{
	import caurina.transitions.Equations;
	import caurina.transitions.Tweener;
	import flash.display.Loader;
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.TimerEvent;
	import flash.net.URLRequest;
	import flash.text.TextField;
	import flash.utils.Timer;
	import se.svt.caspar.template.CasparTemplate;
	
	public class AdvancedTemplate1 extends CasparTemplate
	{
		//This constant determined how far in the graphics will go.
		private const IN_X_POSITION:Number = 250;
		//Defining the custom parameters "time" and "image" so that it will be exposed for the rundown.
		private const customParameterDescription:XML = 	
														<parameters>
														   <parameter id="time" type="int" info="The time to count down in seconds" />
														   <parameter id="image" type="string" info="The path to the image to load" />
														</parameters>;
		
		//The "plate" movie clip on the stage that contains the count down fields with background
		public var plate:MovieClip;		
		//The image container on the stage
		public var imageContainer:MovieClip;																
		//a timer instance for the count down
		private var _timer:Timer;
		//flags used to check that "time" has been set before the template is visible. 
		private var _isPlaying:Boolean = false;
		private var _timeIsSet:Boolean = false;
		//references to the four TextFields that displays the time
		private var _secondsField1:TextField;
		private var _secondsField2:TextField;
		private var _minutesField1:TextField;
		private var _minutesField2:TextField;
		
		//Keeps track of the current remaining seconds
		private var _currentSeconds:int = 0;

		public function AdvancedTemplate1()
		{
			//Set the timer to 1000 ms
			_timer = new Timer(1000);
			//listen for the tick
			_timer.addEventListener(TimerEvent.TIMER, onTick);
			//set references to the TextFields that are prefixed with the letter "x" so they won't be converted to CasparTextFields
			_secondsField1 = plate.xseconds1;
			_secondsField2 = plate.xseconds2;
			_minutesField1 = plate.xminutes1;
			_minutesField2 = plate.xminutes2;
			
			//empty the text fields
			_minutesField1.text = _minutesField2.text = _secondsField1.text = _secondsField2.text = "";
			
			//uncomment lines below to test in flash GUI
			
			/*
			var testxml:XML = new XML(	
										<templateData>
											<componentData id='time'>
												<data id='text' value="15" />
											</componentData>
											<componentData id='image'>
												<data id='text' value="time.png" />
											</componentData>
										</templateData>
									);
			
			SetData(testxml);
			Play();
			*/
			
		}
		
		//overridden preDispose that will be called just before the template is removed by the template host. Allows us to clean up.
		override public function preDispose():void 
		{
			//dispose the timer
			_timer.stop();
			_timer.removeEventListener(TimerEvent.TIMER, onTick);
			_timer = null;
			//dispose the tweener engine
			Tweener.removeAllTweens();
		}
		
		//overridden Stop that initiates the outro animation. IMPORTANT, it is very important when you override Stop to later call super.Stop() or removeTemplate()
		override public function Stop():void 
		{
			//do the outro animation
			outroAnimation();
		}
		
		//overridden Play that will initiate the intro animation and start the timer if there is data
		override public function Play():void 
		{
			//flag that there has been a play command
			_isPlaying = true;
			//if the time has been set
			if (_timeIsSet)
			{
				//set the initial time
				setTime(_currentSeconds);
				//decrease current seconds
				_currentSeconds--;
				//start the timer
				_timer.start();
				//do the intro animation
				introAnimation();
			}
		}
		
		//overridden SetData to be able to recieve data to out custom parameter
		override public function SetData(xmlData:XML):void 
		{
			//loop trough the incoming xml data to find if there is data for out time parameter
			for each (var element:XML in xmlData.children())
			{
				//seems to be our data
				if (element.@id == "time") 		
				{
					//check so it's not empty
					if (element.data.@value != "")
					{
						//check so that the data is a positive integer
						if (int(element.data.@value) > 0)
						{
							//set the current seconds to the value recieved
							_currentSeconds = int(element.data.@value);
							//flag that there now is data available
							if (!_timeIsSet)	_timeIsSet = true;
							//If there has been a Play command already trigger it again since we now has data
							if (_isPlaying)
							{
								Play();
							}
						}
					}
				}
				//we have an image to load
				else if (element.@id == "image") 		
				{
					//check so it's not empty
					if (element.data.@value != "")
					{
						//load it!
						loadImage(element.data.@value);
					}
				}
				
			}
			//the line below is needed if there is other components in the template that needs data
			//super.SetData(xmlData);
		}
		
		private function loadImage(imageURL:String):void
		{
			//make the container invisible
			imageContainer.alpha = 0;
			//create the loader object and load the image
			var loader:Loader = new Loader();
			loader.contentLoaderInfo.addEventListener(Event.COMPLETE, onImageLoaded);
			loader.load(new URLRequest(imageURL));
		}
		
		private function onImageLoaded(e:Event):void 
		{
			//add the image to the container display list
			imageContainer.addChild(e.currentTarget.content);
			//fade up
			Tweener.addTween(imageContainer, { alpha: 1, time: 7, transition: Equations.easeOutCirc } );
		}
		
		//will be called every second
		private function onTick(e:TimerEvent):void 
		{
			//if there is seconds left to count down...
			if (_currentSeconds > 0)
			{
				//update the text fields
				setTime(_currentSeconds);
				//decrease timer
				_currentSeconds --;
			}
			//count down finished
			else
			{
				//update the text fields
				setTime(_currentSeconds);
				//stop the timer
				_timer.removeEventListener(TimerEvent.TIMER, onTick);
				//wait 3 seconds and then call Stop()
				Tweener.addTween(this, { time: 3, onComplete: Stop });
			}
		}
		
		//updates the text fields
		private function setTime(time:int):void
		{
			//get minutes
			var minutes:String = String(getMinutes(time));
			//get seconds
			var seconds:String = String(getRemainingSeconds(time));
			//add a zero if the value < 9
			if (int(minutes) < 10) minutes = "0" + minutes;
			if (int(seconds) < 10) seconds = "0" + seconds;
			//set the text
			_secondsField1.text = seconds.charAt(0);
			_secondsField2.text = seconds.charAt(1);
			_minutesField1.text = minutes.charAt(0);;
			_minutesField2.text = minutes.charAt(1);;
			
		}
		
		//get minutes from seconds
		function getMinutes(seconds):int
		{
			return Math.floor(seconds/60);
		}
		
		//get remaining seconds
		function getRemainingSeconds(seconds:int):int
		{
			return (seconds%60);
		}
		
		//animate the graphics from left to right
		private function introAnimation():void
		{
			Tweener.addTween(plate, { time: .7, x: IN_X_POSITION, transition: Equations.easeOutSine } );
		}
		
		//animate the graphics from right to left
		private function outroAnimation():void
		{
			//IMPORTANT! The removeTemplate method will be called when the animation is finished. This will allow for the template host to properly dispose the template.
			Tweener.addTween(plate, { time: .7, x: -315, transition: Equations.easeInSine, onComplete: removeTemplate } );
		}
	}

}