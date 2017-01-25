//sousVideItem.cpp

#include "Arduino.h"
#include "sousVideItem.h"

SousVideItem::SousVideItem(){
  foodType = "N/A";
  foodSubType = "N/A";
  cookTime = -1;
  cookTemp = -1; 
}
  
SousVideItem::SousVideItem(String fdTyp, String fdSbTyp, float ckTm, float ckTmp){
  foodType = fdTyp;
  foodSubType = fdSbTyp;
  cookTime = ckTm;
  cookTemp = ckTmp;  
}

String SousVideItem::begin() {
	return "Helloworld";
}

String SousVideItem::getFoodType() {
  return this->foodType;
}

String SousVideItem::getFoodSubType() {
  return this->foodSubType;
}

float SousVideItem::getCookTemp() {
  return this->cookTemp;
}

float SousVideItem::getCookTime() {
  return this->cookTime;
}

void SousVideItem::setFoodType(String fdType) {
  this->foodType = fdType;
}

void SousVideItem::setFoodSubType(String fdSubType) {
  this->foodSubType = fdSubType;
}

void SousVideItem::setCookTemp(float ckTemp) {
  this->cookTemp = ckTemp;
}

void SousVideItem::setCookTime(float ckTime) {
  this->cookTime = ckTime;
}

