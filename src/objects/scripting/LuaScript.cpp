#include "LuaScript.h"

//--------------------------------------------------------------
LuaScript::LuaScript() : PatchObject(){

    this->numInlets  = 1;
    this->numOutlets = 2;

    _inletParams[0] = new vector<float>();      // data

    _outletParams[0] = new ofTexture();         // output
    _outletParams[1] = new ofxLua();            // lua script reference (for keyboard and mouse events on external windows)

    for(int i=0;i<this->numInlets;i++){
        this->inletsConnected.push_back(false);
    }

    scriptLoaded        = false;
    isNewObject         = false;

    isGUIObject         = true;
    isOverGui           = true;

    fbo = new ofFbo();
    fbo->allocate(1280,720,GL_RGBA);

    kuro = new ofImage();
    kuro->load("images/kuro.jpg");

    scaleH = 0.0f;

    mosaicTableName = "_mosaic_data_table";
    tempstring      = "";
}

//--------------------------------------------------------------
void LuaScript::newObject(){
    this->setName("lua script");
    this->addInlet(VP_LINK_ARRAY,"data");
    this->addOutlet(VP_LINK_TEXTURE);
    this->addOutlet(VP_LINK_ARRAY);
}

//--------------------------------------------------------------
void LuaScript::setupObjectContent(shared_ptr<ofAppBaseWindow> &mainWindow){
    // init lua
    lua.init(true);
    lua.addListener(this);

    watcher.start();

    if(filepath != "none"){
        loadScript(filepath);
    }else{
        isNewObject = true;
    }

    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
    gui->setAutoDraw(false);
    gui->setUseCustomMouse(true);
    gui->setWidth((this->width/3 * 2) + 1);
    gui->onButtonEvent(this, &LuaScript::onButtonEvent);

    loadButton = gui->addButton("OPEN");
    loadButton->setUseCustomMouse(true);

    editButton = gui->addButton("EDIT");
    editButton->setUseCustomMouse(true);

    gui->setPosition(this->width/3,this->height - (loadButton->getHeight()*2));
}

//--------------------------------------------------------------
void LuaScript::updateObjectContent(){
    // GUI
    gui->update();
    loadButton->update();
    editButton->update();

    while(watcher.waitingEvents()) {
        pathChanged(watcher.nextEvent());
    }

    ///////////////////////////////////////////
    // LUA UPDATE
    if(scriptLoaded){
        // receive external data
        if(this->inletsConnected[0]){
            for(int i=0;i<static_cast<vector<float> *>(_inletParams[0])->size();i++){
                lua_getglobal(lua, "_updateMosaicData");
                lua_pushnumber(lua,i+1);
                lua_pushnumber(lua,static_cast<vector<float> *>(_inletParams[0])->at(i));
                lua_pcall(lua,2,0,0);
            }
        }
        // update lua state
        lua.scriptUpdate();
        // send script reference (for events)
        *static_cast<ofxLua *>(_outletParams[1]) = lua;
    }
    ///////////////////////////////////////////
}

//--------------------------------------------------------------
void LuaScript::drawObjectContent(ofxFontStash *font){
    ofSetColor(255);
    ofEnableAlphaBlending();
    ///////////////////////////////////////////
    // LUA DRAW
    fbo->begin();
    if(scriptLoaded){
        ofPushView();
        ofPushStyle();
        ofPushMatrix();
        lua.scriptDraw();
        ofPopMatrix();
        ofPopStyle();
        ofPopView();
    }else{
        kuro->draw(0,0,fbo->getWidth(),fbo->getHeight());
    }
    fbo->end();
    *static_cast<ofTexture *>(_outletParams[0]) = fbo->getTexture();
    ///////////////////////////////////////////
    scaleH = (OBJECT_WIDTH/fbo->getWidth())*fbo->getHeight();
    static_cast<ofTexture *>(_outletParams[0])->draw(0,OBJECT_HEIGHT/2 - scaleH/2,OBJECT_WIDTH,scaleH);
    // GUI
    gui->draw();
    ofDisableAlphaBlending();
}

//--------------------------------------------------------------
void LuaScript::removeObjectContent(){
    ///////////////////////////////////////////
    // LUA EXIT
    lua.scriptExit();
    lua.clear();
    ///////////////////////////////////////////
}

//--------------------------------------------------------------
void LuaScript::mouseMovedObjectContent(ofVec3f _m){
    gui->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    loadButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    editButton->setCustomMousePos(static_cast<int>(_m.x - this->getPos().x),static_cast<int>(_m.y - this->getPos().y));
    isOverGui = loadButton->hitTest(_m-this->getPos());
}

//--------------------------------------------------------------
void LuaScript::dragGUIObject(ofVec3f _m){
    if(!isOverGui){
        ofNotifyEvent(dragEvent, nId);

        box->setFromCenter(_m.x, _m.y,box->getWidth(),box->getHeight());
        headerBox->set(box->getPosition().x,box->getPosition().y,box->getWidth(),headerHeight);

        x = box->getPosition().x;
        y = box->getPosition().y;

        for(int j=0;j<outPut.size();j++){
            outPut[j]->linkVertices[0].move(outPut[j]->posFrom.x,outPut[j]->posFrom.y);
            outPut[j]->linkVertices[1].move(outPut[j]->posFrom.x+20,outPut[j]->posFrom.y);
        }
    }
}

//--------------------------------------------------------------
void LuaScript::loadScript(string scriptFile){

    filepath = scriptFile;

    lua.scriptExit();
    lua.init(true);
    lua.doScript(filepath, true);

    // inject incoming data vector to lua
    string tempstring = mosaicTableName+" = {}";
    lua.doString(tempstring);
    tempstring = "function _updateMosaicData(i,data) "+mosaicTableName+"[i] = data  end";
    lua.doString(tempstring);

    scriptLoaded = lua.isValid();

    ///////////////////////////////////////////
    // LUA SETUP
    if(scriptLoaded){
        watcher.removeAllPaths();
        watcher.addPath(filepath);
        lua.scriptSetup();
    }
    ///////////////////////////////////////////

}

//--------------------------------------------------------------
void LuaScript::onButtonEvent(ofxDatGuiButtonEvent e){
    if(e.target == loadButton){
        ofFileDialogResult openFileResult= ofSystemLoadDialog("Select a lua script");
        if (openFileResult.bSuccess){
            ofFile file (openFileResult.getPath());
            if (file.exists()){
                string fileExtension = ofToUpper(file.getExtension());
                if(fileExtension == "LUA") {
                    loadScript(file.getAbsolutePath());
                }
            }
        }
    }else if(e.target == editButton){
        if(filepath != "none" && scriptLoaded){
            string cmd = "";
#ifdef TARGET_LINUX
            cmd = "atom "+filepath;
#elif defined(TARGET_OSX)
            cmd = "open -a /Applications/Atom.app "+filepath;
#elif defined(TARGET_WIN32)
            cmd = "atom "+filepath;
#endif
            if(system(cmd.c_str()) != 0){ // error
                ofSystemAlertDialog("Mosaic works better with Atom [https://atom.io/] text editor, and it seems you do not have it installed on your system. Opening script with default text editor!");
#ifdef TARGET_LINUX
                cmd = "nano "+filepath;
#elif defined(TARGET_OSX)
                cmd = "open -a /Applications/TextEdit.app "+filepath;
#elif defined(TARGET_WIN32)
                cmd = "start "+filepath;
#endif
                system(cmd.c_str());
            }
        }
    }
}

//--------------------------------------------------------------
void LuaScript::pathChanged(const PathWatcher::Event &event) {
    switch(event.change) {
        case PathWatcher::CREATED:
            //ofLogVerbose(PACKAGE) << "path created " << event.path;
            break;
        case PathWatcher::MODIFIED:
            //ofLogVerbose(PACKAGE) << "path modified " << event.path;
            loadScript(event.path);
            break;
        case PathWatcher::DELETED:
            //ofLogVerbose(PACKAGE) << "path deleted " << event.path;
            return;
        default: // NONE
            return;
    }

}

//--------------------------------------------------------------
void LuaScript::errorReceived(std::string& msg) {
    //ofLogNotice() << "got a script error: " << msg;
}
