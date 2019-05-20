/*==============================================================================

    ofxVisualProgramming: A visual programming patching environment for OF

    Copyright (c) 2018 Emanuele Mazza aka n3m3da <emanuelemazza@d3cod3.org>

    ofxVisualProgramming is distributed under the MIT License.
    This gives everyone the freedoms to use ofxVisualProgramming in any context:
    commercial or non-commercial, public or private, open or closed source.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    See https://github.com/d3cod3/ofxVisualProgramming for documentation

==============================================================================*/

#include "InharmonicityExtractor.h"

//--------------------------------------------------------------
InharmonicityExtractor::InharmonicityExtractor() : PatchObject(){

    this->numInlets  = 1;
    this->numOutlets = 1;

    _inletParams[0] = new vector<float>();  // RAW Data

    _outletParams[0] = new float(); // inharmonicity
    *(float *)&_outletParams[0] = 0.0f;

    this->initInletsState();

    bufferSize = 1024;
    spectrumSize = (bufferSize/2) + 1;

    arrayPosition = bufferSize + spectrumSize + MELBANDS_BANDS_NUM + DCT_COEFF_NUM + HPCP_SIZE + TRISTIMULUS_BANDS_NUM + 5;
}

//--------------------------------------------------------------
void InharmonicityExtractor::newObject(){
    this->setName("inharmonicity extractor");
    this->addInlet(VP_LINK_ARRAY,"data");
    this->addOutlet(VP_LINK_NUMERIC,"inharmonicity");
}

//--------------------------------------------------------------
void InharmonicityExtractor::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    ofxXmlSettings XML;

    if (XML.loadFile(patchFile)){
        if (XML.pushTag("settings")){
            bufferSize = XML.getValue("buffer_size",0);
            spectrumSize = (bufferSize/2) + 1;
            arrayPosition = bufferSize + spectrumSize + MELBANDS_BANDS_NUM + DCT_COEFF_NUM + HPCP_SIZE + TRISTIMULUS_BANDS_NUM + 5;
            XML.popTag();
        }
    }

    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setAutoDraw(false);
    gui->setWidth(this->width);

    rPlotter = gui->addValuePlotter("",0.0f,1.0f);
    rPlotter->setDrawMode(ofxDatGuiGraph::LINES);
    rPlotter->setSpeed(1);

    gui->setPosition(0,this->height-rPlotter->getHeight());

}

//--------------------------------------------------------------
void InharmonicityExtractor::updateObjectContent(map<int,PatchObject*> &patchObjects, ofxThreadedFileDialog &fd){
    gui->update();
    rPlotter->setValue(*(float *)&_outletParams[0]);

  if(this->inletsConnected[0] && !static_cast<vector<float> *>(_inletParams[0])->empty()){
      *(float *)&_outletParams[0] = static_cast<vector<float> *>(_inletParams[0])->at(arrayPosition);
  }
}

//--------------------------------------------------------------
void InharmonicityExtractor::drawObjectContent(ofxFontStash *font){
    ofSetColor(255);
    ofEnableAlphaBlending();
    gui->draw();
    font->draw(ofToString(*(float *)&_outletParams[0]),this->fontSize,this->width/2,this->headerHeight*2.3);
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void InharmonicityExtractor::removeObjectContent(){

}
