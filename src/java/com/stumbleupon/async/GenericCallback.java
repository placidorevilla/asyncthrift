package com.stumbleupon.async;

public class GenericCallback implements Callback<Object, Object> {
  native public Object call(Object arg);
  long native_object;
}

