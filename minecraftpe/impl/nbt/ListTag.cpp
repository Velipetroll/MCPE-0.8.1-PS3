#include <nbt/ListTag.hpp>
#include <util/input/IDataInput.hpp>
#include <util/output/IDataOutput.hpp>
#include <nbt/FloatTag.hpp>
#include <stdio.h>
#include <string>

ListTag::ListTag(void) : Tag(""), value(){
	this->tagType = 1;
}

ListTag::ListTag(const std::string& s) : Tag(s), value(){
	this->tagType = 1;
}

void ListTag::write(IDataOutput* out){
	if(this->value.size() > 0 && this->value[0]){
		this->tagType = this->value[0]->getId();
	}else{
		this->tagType = 1;
	}

	out->writeByte(this->tagType);
	out->writeInt((int32_t)this->value.size());
	for(Tag* t : this->value){
		if (t) t->write(out);
	}
}

void ListTag::load(IDataInput* in){
	this->tagType = in->readByte();
	int32_t cnt = in->readInt();

	// BLINDAJE: Si 'cnt' es negativo o corrupto, dejamos la lista vacía en lugar de colgar la PS3
	if (cnt <= 0 || cnt > 100000) {
		if (cnt < 0) {
			printf("[ListTag] BLOQUEADO intento de lista con elementos negativos (%d).\n", cnt);
			fflush(stdout);
		}
		return;
	}

	for(int32_t i = 0; i < cnt; ++i){
		Tag* t = Tag::newTag(this->tagType, Tag::NullString);
		if (t) {
			t->load(in);
			this->value.push_back(t);
		} else {
			break;
		}
	}
}

int32_t ListTag::getId(void) const{
	return 9;
}

std::string ListTag::toString(void){
	char buf[64];
	snprintf(buf, sizeof(buf), "%d entries of type ", (int)this->value.size());
	return std::string(buf) + Tag::getTagName(this->tagType);
}

Tag* ListTag::copy(void){
	ListTag* tg = new ListTag(this->getName());
	tg->tagType = this->tagType;
	for(Tag* t : this->value){
		if (t) {
			Tag* cp = t->copy();
			tg->value.push_back(cp);
		}
	}
	return tg;
}

bool_t ListTag::equals(const Tag& v){
	const ListTag* tg = (const ListTag*) &v;
	bool_t eq = Tag::equals(v);
	if(eq){
		if(tg->value.size() == this->value.size()){
			for(size_t i = 0; i < this->value.size(); ++i){
				Tag* t = this->value[i];
				const Tag* t2 = tg->value[i];
				if(!t || !t2 || !t->equals(*t2)) return 0;
			}
			return 1;
		}
		return 0;
	}
	return eq;
}

void ListTag::deleteChildren(void){
	for(Tag* t : this->value){
		if(t){
			t->deleteChildren();
			delete t;
		}
	}
	this->value.clear();
}

void ListTag::print(const std::string& s, PrintStream& ps){
	Tag::print(s, ps);
	std::string v14 = s + "{\n";
	for(Tag* t : this->value){
		if (t) t->print(s + "   ", ps);
	}
	v14 += "}";
}

float ListTag::getFloat(int32_t n){
	if(n < 0 || n >= (int32_t)this->value.size()){
		this->field_0 |= 1;
		return 0.0f;
	}
	Tag* t = this->value[n];
	if(t){
		if(t->getId() == 5){
			FloatTag* ft = (FloatTag*) t;
			return ft->value;
		}
		this->field_0 |= 2;
	}
	return 0.0f;
}

void ListTag::add(Tag* t){
	if (t) {
		this->tagType = t->getId();
		this->value.push_back(t);
	}
}
