/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
   University of Utah.


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
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <Modules/Legacy/Fields/CreateImage.h>
#include <Core/Algorithms/Base/AlgorithmBase.h>
#include <Core/Algorithms/Legacy/Fields/CreateMesh/CreateImageAlgo.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/Legacy/Field/Field.h>

using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Fields;
using namespace SCIRun::Modules::Fields;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun;

const AlgorithmParameterName CreateImage::Width("Width");
const AlgorithmParameterName CreateImage::Height("Height");
const AlgorithmParameterName CreateImage::Depth("Depth");
const AlgorithmParameterName CreateImage::PadPercent("PadPercent");

const AlgorithmParameterName CreateImage::Mode("Mode");
const AlgorithmParameterName CreateImage::Axis("Axis");

const AlgorithmParameterName CreateImage::CenterX("CenterX");
const AlgorithmParameterName CreateImage::CenterY("CenterY");
const AlgorithmParameterName CreateImage::CenterZ("CenterZ");

const AlgorithmParameterName CreateImage::NormalX("NormalX");
const AlgorithmParameterName CreateImage::NormalY("NormalY");
const AlgorithmParameterName CreateImage::NormalZ("NormalZ");

const AlgorithmParameterName CreateImage::Position("Position");

const AlgorithmParameterName CreateImage::DataLocation("DataLocation");

MODULE_INFO_DEF(CreateImage, NewField, SCIRun)

CreateImage::CreateImage() : Module(staticInfo_)
{
  INITIALIZE_PORT(InputField);
  INITIALIZE_PORT(SizeMatrix);
  INITIALIZE_PORT(OVMatrix);
  INITIALIZE_PORT(OutputField);
}

void CreateImage::setStateDefaults()
{
  auto state=get_state();
  
  state->setValue(Width, 20);
  state->setValue(Height, 20);
  state->setValue(Depth, 2);
  state->setValue(PadPercent, 0.0);
  
  state->setValue(Mode, std::string("Mannual"));
  state->setValue(Axis, std::string("X"));
  
  state->setValue(CenterX, 0);
  state->setValue(CenterY, 0);
  state->setValue(CenterZ, 0);
  
  state->setValue(NormalX, 1);
  state->setValue(NormalY, 1);
  state->setValue(NormalZ, 1);
  
  state->setValue(Position, 0);
  state->setValue(Index, 0);
  
  state->setValue(DataLocation, std::string("Node(linear basis)"));
}

void CreateImage::execute()
{
  auto inputField = getOptionalInput(InputField);
  auto sizeMatrix = getOptionalInput(SizeMatrix);
  auto oVMatrix = getOptionalInput(OVMatrix);
  
  FieldHandle output;
  
  auto state = get_state();
  std::string axisInput = state->getValue(Axis).toString();
  
  int axisTemp;
  if(axisInput=="X")
    axisTemp=0;
  else if(axisInput=="Y")
    axisTemp=1;
  else if(axisInput=="Z")
    axisTemp=2;
  else if(axisInput=="Custom")
    axisTemp=3;
  
  auto axis=std::min(2, std::max(0, axisTemp));
  
  Transform trans;
  trans.load_identity();
  
  
  // checking for input matrices
  if(sizeMatrix)
   {
   if(sizeMatrix->rows()==1 && sizeMatrix->cols()==1)
   {
   //double* data=sizeMatrix->getDatapointer();
   const unsigned int size1= static_cast<unsigned int>((*sizeMatrix)(0,0));
   const unsigned int size2= static_cast<unsigned int>((*sizeMatrix)(0,0));
   get_state()->setValue(Width, size1);
   get_state()->setValue(Height, size2);
     
   }
   else if(sizeMatrix->rows()==2 && sizeMatrix->cols()==1)
   {
   //double* data=sizeMatrix->get_data_pointer();
   unsigned int size1= static_cast<unsigned int>((*sizeMatrix)(0,0));
   unsigned int size2= static_cast<unsigned int>((*sizeMatrix)(0,1));
   get_state()->setValue(Width, size1);
   get_state()->setValue(Height, size2);
     
   }
   else
   {
   THROW_ALGORITHM_INPUT_ERROR("Image Size matrix must have only 1 or 2 elements");
   }
   }
  
  if(oVMatrix)
  {
    if(oVMatrix->nrows()!=2 || oVMatrix->ncols()!=3)
    {
      THROW_ALGORITHM_INPUT_ERROR("Custom Center and Nomal matrix must be of size 2x3. The Center is the first row and the normal is the second");
    }
    customCenter=Point((*oVMatrix)(0,0),(*oVMatrix)(0,1),(*oVMatrix)(0,2));
    customNormal=Point((*oVMatrix)(1,0),(*oVMatrix)(1,1),(*oVMatrix)(1,2));
    customNormal.safe_normalize();
    
  }
  
  double angle=0;
  Vector axisVector(0.0,0.0,1.0);
  switch(axis)
  {
    case 0:
      angle = M_PI * -0.5;
      axisVector = Vector(0.0, 1.0, 0.0);
      break;
      
    case 1:
      angle = M_PI * 0.5;
      axisVector = Vector(1.0, 0.0, 0.0);
      break;
      
    case 2:
      angle = 0.0;
      axisVector = Vector(0.0, 0.0, 1.0);
      break;
      
    default:
      break;
  }
  
  trans.pre_rotate(angle,axisVector);
  
  if(axis==3)
  {
    customNormal=Vector(get(Parameters::NormalX).toDouble(),get(Parameters::NormalY).toDouble(),get(Parameters::NormalZ).toDouble());
    Vector tempNormal(-customNormal);
    Vector fakey(Cross(Vector(0.0,0.0,1.0),tempNormal));
    
    if(fakey.length2()<1.0e-6)
      fakey=Cross(Vector(1.0,0.0,0.0),tempNormal);
    Vector fakex(Cross(tempNormal,fakey));
    tempNormal.safe_normalize();
    fakex.safe_normalize();
    fakey.safe_normalize();
    double dg=1.0;
    
    if(inputField)
    {
      BBox box=inputField->vmesh()->get_bounding_box();
      Vector diag(box.diagonal());
      dg=diag.maxComponent();
      trans.pre_scale(Vector(dg,dg,dg));
    }
    Transform trans2;
    trans2.load_identity();
    trans2.load_basis(Point(0,0,0), fakex, fakey, tempNormal);
    trans2.invert();
    trans.change_basis(trans2);
    customCenter=Point(get(Parameters::CenterX).toDouble(),get(Parameters::CenterY).toDouble(),get(Parameters::CenterZ).toDouble());
    trans.pre_translate(Vector(customCenter));
  }
  
  DataTypeEnum datatype;
  unsigned int width,height, depth;
  
  if(!inputField)
  {
    datatype=SCALAR;
    // Create blank mesh.
    width=std::max(2,get(Parameters::Width).toInt());
    height=std::max(2,get(Parameters::Height).toInt());
  }
  else
   {
   datatype = SCALAR;
   FieldInformation fi(inputField);
   if (fi.is_tensor())
   {
   datatype = TENSOR;
   }
   else if (fi.is_vector())
   {
   datatype = VECTOR;
   }
   
   int basis_order=1;
   if(getOption(Parameters::Mode)=="Auto")
   {
   // Guess at the size of the sample plane.
   // Currently we have only a simple algorithm for LatVolFields.
   
   if (fi.is_latvolmesh())
   {
   VMesh *lvm = inputField->vmesh();
   basis_order = inputField->vfield()->basis_order();
   
   switch(axis)
   {
   case 0:
   width = std::max(2, (int)lvm->get_nj());
   Width.set(width);
   height = std::max(2, (int)lvm->get_nk());
   Height.set(height);
   depth = std::max(2, (int)lvm->get_ni());
   if( basis_order == 0 )
   {
   Depth.set( depth - 1 );
   }
   else
   {
   Depth.set( depth );
   }
   //TCLInterface::execute(get_id()+" edit_scale");
   break;
   case 1:
   width =  std::max(2, (int)lvm->get_ni());
   Width.set( width );
   height =  std::max(2, (int)lvm->get_nk());
   Height.set( height );
   depth = std::max(2, (int)lvm->get_nj());
   if( basis_order == 0 )
   {
   Depth.set( depth - 1 );
   }
   else
   {
   Depth.set( depth ););
   }
   //TCLInterface::execute(get_id()+" edit_scale");
   break;
   case 2:
   width =  std::max(2, (int)lvm->get_ni());
   Width.set( width );
   height =  std::max(2, (int)lvm->get_nj());
   Height.set( height );
   depth =  std::max(2, (int)lvm->get_nk());
   if( basis_order == 0 )
   {
   Depth.set( depth - 1 );
   }
   else
   {
   Depth.set( depth );
   }
   //TCLInterface::execute(get_id()+" edit_scale");
   break;
   default:
   warning("Custom axis, resize manually.");
   sizex = std::max(2, get(Parameters::Width).toInt());
   sizey = std::max(2, get(Parameters::Height).toInt());
   break;
   }
   }
   else
   {
   warning("No autosize algorithm for this field type, resize manually.");
   sizex = std::max(2, get(Parameters::Width).toInt());
   sizey = std::max(2, get(Parameters::Height).toInt());
   Mode.set("Manual");
   //TCLInterface::execute(get_id()+" edit_scale");
   }
   }
   else
   {
   // Create blank mesh.
   width = std::max(2, get(Parameters::Width.toInt()));
   height = std::max(2, get(Parameters::Height.toInt()));
   }
   
   if(axis!=3)
   {
   BBox box = inputField->vmesh()->get_bounding_box();
   Vector diag(box.diagonal());
   trans.pre_scale(diag);
   
   Point loc(box.center());
   Position.reset();
   double dist;
   if (getOption(Parameters::Mode)=="Manual")
   {
   dist = get(Parameters::Position).toDouble()/2.0;
   }
   else
   {
   if( basis_order == 0 )
   {
   dist = double( get(Parameters::Index).toInt())/ get(Parameters::Depth).toDouble() + 0.5/get(Parameters::Depth).toDouble();
   Position.set( ( dist) * 2.0 );
   }
   else
   {
   dist = double( get(Parameters::Index)toInt() )/ get(Parameters::Depth);
   Position.set( ( dist) * 2.0 );
   }
   }
   switch (axis)
   {
   case 0:
   loc.x(loc.x() + diag.x() * dist);
   break;
   
   case 1:
   loc.y(loc.y() + diag.y() * dist);
   break;
   
   case 2:
   loc.z(loc.z() + diag.z() * dist);
   break;
   
   default:
   break;
   }
   trans.pre_translate(Vector(loc));
   }
   }
  
  Point minb(-0.5, -0.5, 0.0);
  Point maxb(0.5, 0.5, 0.0);
  Vector diag((Vector(maxb) - Vector(minb)) * (get(Parameters::PadPercent).toDouble()/100.0));
  minb -= diag;
  maxb += diag;
  
  int basis_order;
  if (getOption(Parameters::DataLocation) == "Nodes") basis_order = 1;
  else if (getOption(Parameters::DataLocation) == "Faces") basis_order = 0;
  else if (getOption(Parameters::DataLocation) == "None") basis_order = -1;
  /*else
   {
   error("Unsupported data_at location " + getOption(Parameters::DataLocation) + ".");
   AlgorithmOutput result;
   return result;
   }*/
  
  FieldInformation ifi("ImageMesh",basis_order,"double");
  
  if (datatype == VECTOR) ifi.make_vector();
  else if (datatype == TENSOR) ifi.make_tensor();
  
  MeshHandle imagemesh = CreateMesh(ifi,width, height, minb, maxb);
  output = CreateField(ifi,imagemesh);
  
  // Transform field.
  output->vmesh()->transform(trans);
  
    
    sendOutputFromAlgorithm(OutputField, output);
  }
}
