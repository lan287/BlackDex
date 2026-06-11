/*
 * This file is auto-generated.  DO NOT MODIFY.
 */
package top.niunaijun.blackbox.core.system.am;
// Declare any non-default types here with import statements
public interface IBActivityManagerService extends android.os.IInterface
{
  /** Default implementation for IBActivityManagerService. */
  public static class Default implements top.niunaijun.blackbox.core.system.am.IBActivityManagerService
  {
    @Override public top.niunaijun.blackbox.entity.AppConfig initProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException
    {
      return null;
    }
    @Override public void restartProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException
    {
    }
    @Override public void startActivity(android.content.Intent intent, int userId) throws android.os.RemoteException
    {
    }
    @Override public int startActivityAms(int userId, android.content.Intent intent, java.lang.String resolvedType, android.os.IBinder resultTo, java.lang.String resultWho, int requestCode, int flags, android.os.Bundle options) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public int startActivities(int userId, android.content.Intent[] intent, java.lang.String[] resolvedType, android.os.IBinder resultTo, android.os.Bundle options) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public android.content.ComponentName startService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
    {
      return null;
    }
    @Override public int stopService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
    {
      return 0;
    }
    @Override public android.content.Intent bindService(android.content.Intent service, android.os.IBinder binder, java.lang.String resolvedType, int userId) throws android.os.RemoteException
    {
      return null;
    }
    @Override public void unbindService(android.os.IBinder binder, int userId) throws android.os.RemoteException
    {
    }
    @Override public void onStartCommand(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException
    {
    }
    @Override public void onServiceDestroy(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException
    {
    }
    @Override public android.content.Intent sendBroadcast(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
    {
      return null;
    }
    @Override public void onActivityCreated(int taskId, android.os.IBinder token, android.os.IBinder activityRecord) throws android.os.RemoteException
    {
    }
    @Override public void onActivityResumed(android.os.IBinder token) throws android.os.RemoteException
    {
    }
    @Override public void onActivityDestroyed(android.os.IBinder token) throws android.os.RemoteException
    {
    }
    @Override public void onFinishActivity(android.os.IBinder token) throws android.os.RemoteException
    {
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements top.niunaijun.blackbox.core.system.am.IBActivityManagerService
  {
    /** Construct the stub at attach it to the interface. */
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an top.niunaijun.blackbox.core.system.am.IBActivityManagerService interface,
     * generating a proxy if needed.
     */
    public static top.niunaijun.blackbox.core.system.am.IBActivityManagerService asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof top.niunaijun.blackbox.core.system.am.IBActivityManagerService))) {
        return ((top.niunaijun.blackbox.core.system.am.IBActivityManagerService)iin);
      }
      return new top.niunaijun.blackbox.core.system.am.IBActivityManagerService.Stub.Proxy(obj);
    }
    @Override public android.os.IBinder asBinder()
    {
      return this;
    }
    @Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
    {
      java.lang.String descriptor = DESCRIPTOR;
      if (code >= android.os.IBinder.FIRST_CALL_TRANSACTION && code <= android.os.IBinder.LAST_CALL_TRANSACTION) {
        data.enforceInterface(descriptor);
      }
      switch (code)
      {
        case INTERFACE_TRANSACTION:
        {
          reply.writeString(descriptor);
          return true;
        }
      }
      switch (code)
      {
        case TRANSACTION_initProcess:
        {
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          top.niunaijun.blackbox.entity.AppConfig _result = this.initProcess(_arg0, _arg1, _arg2);
          reply.writeNoException();
          _Parcel.writeTypedObject(reply, _result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          break;
        }
        case TRANSACTION_restartProcess:
        {
          java.lang.String _arg0;
          _arg0 = data.readString();
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          this.restartProcess(_arg0, _arg1, _arg2);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_startActivity:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          int _arg1;
          _arg1 = data.readInt();
          this.startActivity(_arg0, _arg1);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_startActivityAms:
        {
          int _arg0;
          _arg0 = data.readInt();
          android.content.Intent _arg1;
          _arg1 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          java.lang.String _arg2;
          _arg2 = data.readString();
          android.os.IBinder _arg3;
          _arg3 = data.readStrongBinder();
          java.lang.String _arg4;
          _arg4 = data.readString();
          int _arg5;
          _arg5 = data.readInt();
          int _arg6;
          _arg6 = data.readInt();
          android.os.Bundle _arg7;
          _arg7 = _Parcel.readTypedObject(data, android.os.Bundle.CREATOR);
          int _result = this.startActivityAms(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7);
          reply.writeNoException();
          reply.writeInt(_result);
          break;
        }
        case TRANSACTION_startActivities:
        {
          int _arg0;
          _arg0 = data.readInt();
          android.content.Intent[] _arg1;
          _arg1 = data.createTypedArray(android.content.Intent.CREATOR);
          java.lang.String[] _arg2;
          _arg2 = data.createStringArray();
          android.os.IBinder _arg3;
          _arg3 = data.readStrongBinder();
          android.os.Bundle _arg4;
          _arg4 = _Parcel.readTypedObject(data, android.os.Bundle.CREATOR);
          int _result = this.startActivities(_arg0, _arg1, _arg2, _arg3, _arg4);
          reply.writeNoException();
          reply.writeInt(_result);
          break;
        }
        case TRANSACTION_startService:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          android.content.ComponentName _result = this.startService(_arg0, _arg1, _arg2);
          reply.writeNoException();
          _Parcel.writeTypedObject(reply, _result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          break;
        }
        case TRANSACTION_stopService:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          int _result = this.stopService(_arg0, _arg1, _arg2);
          reply.writeNoException();
          reply.writeInt(_result);
          break;
        }
        case TRANSACTION_bindService:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          android.os.IBinder _arg1;
          _arg1 = data.readStrongBinder();
          java.lang.String _arg2;
          _arg2 = data.readString();
          int _arg3;
          _arg3 = data.readInt();
          android.content.Intent _result = this.bindService(_arg0, _arg1, _arg2, _arg3);
          reply.writeNoException();
          _Parcel.writeTypedObject(reply, _result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          break;
        }
        case TRANSACTION_unbindService:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          int _arg1;
          _arg1 = data.readInt();
          this.unbindService(_arg0, _arg1);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_onStartCommand:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          int _arg1;
          _arg1 = data.readInt();
          this.onStartCommand(_arg0, _arg1);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_onServiceDestroy:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          int _arg1;
          _arg1 = data.readInt();
          this.onServiceDestroy(_arg0, _arg1);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_sendBroadcast:
        {
          android.content.Intent _arg0;
          _arg0 = _Parcel.readTypedObject(data, android.content.Intent.CREATOR);
          java.lang.String _arg1;
          _arg1 = data.readString();
          int _arg2;
          _arg2 = data.readInt();
          android.content.Intent _result = this.sendBroadcast(_arg0, _arg1, _arg2);
          reply.writeNoException();
          _Parcel.writeTypedObject(reply, _result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
          break;
        }
        case TRANSACTION_onActivityCreated:
        {
          int _arg0;
          _arg0 = data.readInt();
          android.os.IBinder _arg1;
          _arg1 = data.readStrongBinder();
          android.os.IBinder _arg2;
          _arg2 = data.readStrongBinder();
          this.onActivityCreated(_arg0, _arg1, _arg2);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_onActivityResumed:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.onActivityResumed(_arg0);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_onActivityDestroyed:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.onActivityDestroyed(_arg0);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_onFinishActivity:
        {
          android.os.IBinder _arg0;
          _arg0 = data.readStrongBinder();
          this.onFinishActivity(_arg0);
          reply.writeNoException();
          break;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
      return true;
    }
    private static class Proxy implements top.niunaijun.blackbox.core.system.am.IBActivityManagerService
    {
      private android.os.IBinder mRemote;
      Proxy(android.os.IBinder remote)
      {
        mRemote = remote;
      }
      @Override public android.os.IBinder asBinder()
      {
        return mRemote;
      }
      public java.lang.String getInterfaceDescriptor()
      {
        return DESCRIPTOR;
      }
      @Override public top.niunaijun.blackbox.entity.AppConfig initProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        top.niunaijun.blackbox.entity.AppConfig _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(packageName);
          _data.writeString(processName);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_initProcess, _data, _reply, 0);
          _reply.readException();
          _result = _Parcel.readTypedObject(_reply, top.niunaijun.blackbox.entity.AppConfig.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public void restartProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeString(packageName);
          _data.writeString(processName);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_restartProcess, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void startActivity(android.content.Intent intent, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, intent, 0);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_startActivity, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public int startActivityAms(int userId, android.content.Intent intent, java.lang.String resolvedType, android.os.IBinder resultTo, java.lang.String resultWho, int requestCode, int flags, android.os.Bundle options) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(userId);
          _Parcel.writeTypedObject(_data, intent, 0);
          _data.writeString(resolvedType);
          _data.writeStrongBinder(resultTo);
          _data.writeString(resultWho);
          _data.writeInt(requestCode);
          _data.writeInt(flags);
          _Parcel.writeTypedObject(_data, options, 0);
          boolean _status = mRemote.transact(Stub.TRANSACTION_startActivityAms, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int startActivities(int userId, android.content.Intent[] intent, java.lang.String[] resolvedType, android.os.IBinder resultTo, android.os.Bundle options) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(userId);
          _data.writeTypedArray(intent, 0);
          _data.writeStringArray(resolvedType);
          _data.writeStrongBinder(resultTo);
          _Parcel.writeTypedObject(_data, options, 0);
          boolean _status = mRemote.transact(Stub.TRANSACTION_startActivities, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.content.ComponentName startService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        android.content.ComponentName _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, intent, 0);
          _data.writeString(resolvedType);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_startService, _data, _reply, 0);
          _reply.readException();
          _result = _Parcel.readTypedObject(_reply, android.content.ComponentName.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public int stopService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        int _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, intent, 0);
          _data.writeString(resolvedType);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_stopService, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readInt();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public android.content.Intent bindService(android.content.Intent service, android.os.IBinder binder, java.lang.String resolvedType, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        android.content.Intent _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, service, 0);
          _data.writeStrongBinder(binder);
          _data.writeString(resolvedType);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_bindService, _data, _reply, 0);
          _reply.readException();
          _result = _Parcel.readTypedObject(_reply, android.content.Intent.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public void unbindService(android.os.IBinder binder, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(binder);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_unbindService, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void onStartCommand(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, proxyIntent, 0);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onStartCommand, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void onServiceDestroy(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, proxyIntent, 0);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onServiceDestroy, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public android.content.Intent sendBroadcast(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        android.content.Intent _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _Parcel.writeTypedObject(_data, intent, 0);
          _data.writeString(resolvedType);
          _data.writeInt(userId);
          boolean _status = mRemote.transact(Stub.TRANSACTION_sendBroadcast, _data, _reply, 0);
          _reply.readException();
          _result = _Parcel.readTypedObject(_reply, android.content.Intent.CREATOR);
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public void onActivityCreated(int taskId, android.os.IBinder token, android.os.IBinder activityRecord) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(taskId);
          _data.writeStrongBinder(token);
          _data.writeStrongBinder(activityRecord);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onActivityCreated, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void onActivityResumed(android.os.IBinder token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onActivityResumed, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void onActivityDestroyed(android.os.IBinder token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onActivityDestroyed, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      @Override public void onFinishActivity(android.os.IBinder token) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeStrongBinder(token);
          boolean _status = mRemote.transact(Stub.TRANSACTION_onFinishActivity, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
    }
    static final int TRANSACTION_initProcess = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    static final int TRANSACTION_restartProcess = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
    static final int TRANSACTION_startActivity = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
    static final int TRANSACTION_startActivityAms = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
    static final int TRANSACTION_startActivities = (android.os.IBinder.FIRST_CALL_TRANSACTION + 4);
    static final int TRANSACTION_startService = (android.os.IBinder.FIRST_CALL_TRANSACTION + 5);
    static final int TRANSACTION_stopService = (android.os.IBinder.FIRST_CALL_TRANSACTION + 6);
    static final int TRANSACTION_bindService = (android.os.IBinder.FIRST_CALL_TRANSACTION + 7);
    static final int TRANSACTION_unbindService = (android.os.IBinder.FIRST_CALL_TRANSACTION + 8);
    static final int TRANSACTION_onStartCommand = (android.os.IBinder.FIRST_CALL_TRANSACTION + 9);
    static final int TRANSACTION_onServiceDestroy = (android.os.IBinder.FIRST_CALL_TRANSACTION + 10);
    static final int TRANSACTION_sendBroadcast = (android.os.IBinder.FIRST_CALL_TRANSACTION + 11);
    static final int TRANSACTION_onActivityCreated = (android.os.IBinder.FIRST_CALL_TRANSACTION + 12);
    static final int TRANSACTION_onActivityResumed = (android.os.IBinder.FIRST_CALL_TRANSACTION + 13);
    static final int TRANSACTION_onActivityDestroyed = (android.os.IBinder.FIRST_CALL_TRANSACTION + 14);
    static final int TRANSACTION_onFinishActivity = (android.os.IBinder.FIRST_CALL_TRANSACTION + 15);
  }
  public static final java.lang.String DESCRIPTOR = "top.niunaijun.blackbox.core.system.am.IBActivityManagerService";
  public top.niunaijun.blackbox.entity.AppConfig initProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException;
  public void restartProcess(java.lang.String packageName, java.lang.String processName, int userId) throws android.os.RemoteException;
  public void startActivity(android.content.Intent intent, int userId) throws android.os.RemoteException;
  public int startActivityAms(int userId, android.content.Intent intent, java.lang.String resolvedType, android.os.IBinder resultTo, java.lang.String resultWho, int requestCode, int flags, android.os.Bundle options) throws android.os.RemoteException;
  public int startActivities(int userId, android.content.Intent[] intent, java.lang.String[] resolvedType, android.os.IBinder resultTo, android.os.Bundle options) throws android.os.RemoteException;
  public android.content.ComponentName startService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException;
  public int stopService(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException;
  public android.content.Intent bindService(android.content.Intent service, android.os.IBinder binder, java.lang.String resolvedType, int userId) throws android.os.RemoteException;
  public void unbindService(android.os.IBinder binder, int userId) throws android.os.RemoteException;
  public void onStartCommand(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException;
  public void onServiceDestroy(android.content.Intent proxyIntent, int userId) throws android.os.RemoteException;
  public android.content.Intent sendBroadcast(android.content.Intent intent, java.lang.String resolvedType, int userId) throws android.os.RemoteException;
  public void onActivityCreated(int taskId, android.os.IBinder token, android.os.IBinder activityRecord) throws android.os.RemoteException;
  public void onActivityResumed(android.os.IBinder token) throws android.os.RemoteException;
  public void onActivityDestroyed(android.os.IBinder token) throws android.os.RemoteException;
  public void onFinishActivity(android.os.IBinder token) throws android.os.RemoteException;
  /** @hide */
  static class _Parcel {
    static private <T> T readTypedObject(
        android.os.Parcel parcel,
        android.os.Parcelable.Creator<T> c) {
      if (parcel.readInt() != 0) {
          return c.createFromParcel(parcel);
      } else {
          return null;
      }
    }
    static private <T extends android.os.Parcelable> void writeTypedObject(
        android.os.Parcel parcel, T value, int parcelableFlags) {
      if (value != null) {
        parcel.writeInt(1);
        value.writeToParcel(parcel, parcelableFlags);
      } else {
        parcel.writeInt(0);
      }
    }
  }
}
