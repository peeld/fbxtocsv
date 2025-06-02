#include "fbxsdk.h"
using namespace fbxsdk;

#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>

void Transforms(std::vector<FbxNode*> &res, FbxNode* parent)
{
    for (int i = 0; i < parent->GetChildCount(); i++)
    {
        FbxNode* child = parent->GetChild(i);
        FbxNodeAttribute *attr = child->GetNodeAttribute();
        if (!attr)
            continue;

        FbxNodeAttribute::EType node_type = attr->GetAttributeType();

        if (node_type == FbxNodeAttribute::eNull ||
            node_type == FbxNodeAttribute::eSkeleton ||
            node_type == FbxNodeAttribute::eCamera)
        {
            res.push_back(child);
        }

        Transforms(res, child);
    }
}


void QueryChannel(FbxAnimCurve *curve, FbxTime& first_frame, FbxTime &last_frame, bool& firstSample)
{
    // Get the first and last frame of the curve

    for (int key_n = 0; key_n < curve->KeyGetCount(); key_n++)
    {
        if (firstSample)
        {
            // First time here, first is last.
            first_frame = curve->KeyGetTime(key_n);
            last_frame = first_frame;
            firstSample = false;
        }
        else
        {
            // New value, update first / last.
            FbxTime t = curve->KeyGetTime(key_n);
            if (t < first_frame) first_frame = t;
            if (t > last_frame) last_frame = t;
        }
    }

}


int main(int argc, char *argv[])
{
    int lFileMajor, lFileMinor, lFileRevision;

    FbxString InFile, OutFile;

    bool query = false;

    std::ofstream outFile;


    switch(argc)
    {
  
    case 3:
        InFile = argv[1];
        outFile.open(argv[2], std::ios::out | std::ios::trunc);
        query = true;
        break;


    default:
            std::cout << "Usage: " << argv[0] << " source.fbx fps start-timecode end-timecode out.fbx" << std::endl;
            return 1;
    }

    // Manager
    FbxManager *manager = FbxManager::Create();
    FbxIOSettings *settings = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(settings);

    // Importer
    FbxImporter *importer = FbxImporter::Create(manager, "");
    importer->Initialize(InFile);

    // Scene
    FbxScene *scene = FbxScene::Create(manager, "myscene");
    importer->Import(scene);
    importer->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);


    //FbxAnimStack *anim_stack = FbxAnimStack::Create(scene, "Base_Stack");
    //FbxAnimLayer *anim_layer = FbxAnimLayer::Create(scene, "Base_Layer");
    //anim_stack->AddMember(anim_layer);

    // https://help.autodesk.com/view/FBX/2017/ENU/?guid=__files_GUID_9481A726_315C_4A58_A347_8AC95C2AF0F2_htm

    FbxTime first_frame, last_frame;

    if (!importer->IsFBX())
    {
        std::cerr << "Could not parse file" << std::endl;
        return 1;
    }

    FbxGlobalSettings& globalSettings = scene->GetGlobalSettings();
    FbxTime::EMode timeMode = globalSettings.GetTimeMode();
    double frameRate = FbxTime::GetFrameRate(timeMode);

    std::cout << "Loaded:          " << InFile.Buffer() << std::endl;
    std::cout << "FBX version:     " << lFileMajor << "." << lFileMinor << "." << lFileRevision << std::endl;
    std::cout << "Animation Stack: " << importer->GetActiveAnimStackName().Buffer() << std::endl;
    std::cout << "Rate:            " << frameRate << std::endl;

    // Find all the transforms in the scene
    std::vector<FbxNode*>  nodes;
    std::vector<FbxAnimCurve*> curves;
    std::vector<std::string> channels;

    Transforms(nodes, scene->GetRootNode());

    bool firstSample = true;

    for (int stack_n = 0; stack_n < scene->GetSrcObjectCount<FbxAnimStack>(); ++stack_n)
    {
        // Stack
        FbxAnimStack* anim_stack = scene->GetSrcObject<FbxAnimStack>(stack_n);
        std::cout << " - Animation Stack: " << anim_stack->GetName() << std::endl;

        int numLayers = anim_stack->GetMemberCount<FbxAnimLayer>();
        for (int layer_n = 0; layer_n < numLayers; ++layer_n)
        {
            // Layer
            FbxAnimLayer* anim_layer = anim_stack->GetMember<FbxAnimLayer>(layer_n);
            std::cout << "   - Animation Layer: " << anim_layer->GetName() << std::endl;

            for(auto node : nodes)
            {
                // Node
                // node->GetName();

                FbxProperty prop = node->GetFirstProperty();
                while (prop.IsValid())
                {
                    // Property
                    FbxAnimCurveNode *curve_node = prop.GetCurveNode();
                    if (curve_node)
                    {
                        // Curve
                        for (unsigned int channel_n = 0; channel_n < curve_node->GetChannelsCount(); channel_n++)
                        {
                            FbxAnimCurve* curve = curve_node->GetCurve(channel_n);
                            if(curve == nullptr || curve->KeyGetCount() == 0) {
								continue;
							}

                            // Get the first and last frame (min/max for all channels)
                            QueryChannel(curve, first_frame, last_frame, firstSample);

                            curves.push_back(curve);
                            std::string name(node->GetName());
                            name += ".";
                            name += prop.GetName().Buffer();
                            name += ".";
                            name += curve_node->GetChannelName(channel_n).Buffer();

                            std::cout << name << " " 
                                << first_frame.GetTimeString(FbxTime::eHours, FbxTime::eFrames,timeMode, FbxTime::eSMPTE) << " "
                                << last_frame.GetTimeString(FbxTime::eHours, FbxTime::eFrames, timeMode, FbxTime::eSMPTE) << std::endl;

                            channels.push_back(name);
                        }
                    }

                    // Add the property to the list
                    prop = node->GetNextProperty(prop);
                }
            }
        }
    }

    std::cout << "First Frame: " << first_frame.GetTimeString(FbxTime::eHours, FbxTime::eFrames, timeMode, FbxTime::eSMPTE).Buffer() << std::endl;
    std::cout << "Last Frame: " << last_frame.GetTimeString(FbxTime::eHours, FbxTime::eFrames, timeMode, FbxTime::eSMPTE).Buffer() << std::endl;
    std::cout << "Channels: " << channels.size() << std::endl;

    outFile << "Frame, Time";
    for(auto channel: channels) { outFile << ", " << channel; }
    outFile << std::endl;

    FbxTime frameStep;
    frameStep.SetTime(0, 0, 0, 1, 0, timeMode);

    int frame = 0;
    for (FbxTime currentTime = first_frame; currentTime <= last_frame; currentTime += frameStep)
    {
        FbxString timeStr = currentTime.GetTimeString(FbxTime::eHours, FbxTime::eFrames, timeMode, FbxTime::eSMPTE);

        outFile << timeStr.Buffer() << ", " << frame;

        // std::cout << frame << " " <<
        //    currentTime.GetTimeString(FbxTime::eHours, FbxTime::eFrames, timeMode, FbxTime::eSMPTE).Buffer() <<
        //    std::endl;

        for(size_t curve_n = 0; curve_n < curves.size(); ++curve_n)
		{
			FbxAnimCurve *curve = curves[curve_n];
            assert(curve != nullptr);

			double value = curve->Evaluate(currentTime);
			outFile << ", " << value;
		}

        outFile << std::endl;


        frame++;

    }

    std::cout << "Processed " << frame << " frames." << std::endl;

    return 0;   
}

