//sousVideItem.h

#ifndef _MY_CLASSES_H
#define _MY_CLASSES_H



class SousVideItem {
	public:
    SousVideItem();
    SousVideItem(String fdTyp, String fdSbTyp, float ckTm, float ckTmp);
		String begin();
    String getFoodType();
    String getFoodSubType();
    float getCookTemp();
    float getCookTime();
    void setFoodType(String fdType);
    void setFoodSubType(String fdSubType);
    void setCookTemp(float ckTemp);
    void setCookTime(float ckTime);
    
  private:
    String foodType, foodSubType;
    float cookTemp, cookTime;
    
};

#endif
