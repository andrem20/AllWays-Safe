import React, { useState, useEffect } from 'react';
import { Users, Key, Settings, Eye, EyeOff, Monitor, ArrowLeft, Clock, Plus, Trash2, Ambulance, IdCard, Search, MapPinned, SquareChartGantt, Pencil} from 'lucide-react';
import { createClient } from '@supabase/supabase-js';

// Supabase config
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL || 'YOUR_SUPABASE_URL';
const supabaseKey = import.meta.env.VITE_SUPABASE_ANON_KEY || 'YOUR_SUPABASE_KEY';
const supabase = createClient(supabaseUrl, supabaseKey);

const LoginPage = ({ onLogin }) => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');

  const handleLogin = async (e) => {
    if (e) e.preventDefault();
    setError('');

    const { data, error } = await supabase
      .from('tmc')
      .select('*')
      .eq('username', username)
      .eq('password', password)
      .single();

    if (error || !data) {
      setError('Invalid username or password');
      return;
    }

    onLogin(data);
  };

  const handleKeyPress = (e) => {
    if (e.key === 'Enter') {
      handleLogin();
    }
  };

  return (
    <div className="min-h-screen bg-gray-50 px-4 flex flex-col">
      <div className="pt-10">
        <h1
          className="text-5xl font-bold italic text-center"
          style={{ color: '#1E90FF' }}
        >
          AllWays Safe
        </h1>
      </div>

      {/* (Login) */}
      <div className="flex flex-1 items-center justify-center">
        <div className="w-full max-w-md bg-white pt-8 pb-12 px-12 rounded-3xl shadow-xl flex flex-col items-center">

          <h2 className="text-2xl font-medium mb-12 text-center text-gray-700">
            Login
          </h2>

          <div className="w-full flex flex-col gap-10">
            
            {/* Username */}
            <div className="flex items-center gap-3 w-full">
              <span className="text-gray-500">
                <Users size={24} />
              </span>
              <input
                type="text"
                placeholder="username"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                onKeyPress={handleKeyPress}
                className="flex-1 py-4 px-6 rounded-full bg-gray-100 text-gray-700 focus:outline-none focus:bg-gray-200 transition"
              />
            </div>

            {/* Password */}
            <div className="flex items-center gap-3 w-full">
              <span className="text-gray-500">
                <Key size={24} />
              </span>
              <input
                type="password"
                placeholder="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                onKeyPress={handleKeyPress}
                className="flex-1 py-4 px-6 rounded-full bg-gray-100 text-gray-700 focus:outline-none focus:bg-gray-200 transition"
              />
            </div>

            {error && (
              <p className="text-red-500 text-sm text-center">
                {error}
              </p>
            )}

            {/* Sign In Button */}
            <button
              onClick={handleLogin}
              className="w-full py-3 rounded-2xl bg-blue-500 text-white font-semibold text-lg hover:bg-blue-600 shadow-lg transition"
            >
              Sign In
            </button>

          </div>
        </div>
      </div>
    </div>
  );

};


// ==========================
// DASHBOARD
// ==========================
const Dashboard = ({ onLogout }) => {
  const [currentPage, setCurrentPage] = useState('home');
  const [currentTime, setCurrentTime] = useState(new Date());

  useEffect(() => {
    const timer = setInterval(() => setCurrentTime(new Date()), 1000);
    return () => clearInterval(timer);
  }, []);

  const formatTime = (date) => {
    return date.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', hour12: false });
  };

  return (
    <div className="min-h-screen bg-gray-50">
      {/* Back Button  */}
        {['setup', 'pedestrians', 'monitor'].includes(currentPage) && (
          <button
            onClick={() => setCurrentPage('home')}
            className="fixed top-4 left-4 z-50 flex items-center justify-center
              w-12 h-12 bg-blue-500 rounded-full shadow-xl
              hover:bg-blue-600 transition-all hover:scale-110"
          >
            <ArrowLeft size={28} color="#ffffff"/>
          </button>
        )}     

      {/* Header */}
      <div className="bg-white shadow-sm px-0 py-4 relative grid grid-cols-3 items-center">
        
        <div></div>

        <h1 className="text-2xl font-bold text-center">
          {currentPage === 'home' && 'Traffic Management Center'}
          {currentPage === 'setup' && 'Set Up'}
          {currentPage === 'pedestrians' && 'Pedestrians'}
          {currentPage === 'monitor' && 'Monitor'}
        </h1>

        {/* Logout  */}
        <div className="flex items-center justify-end gap-4">
          <button
            onClick={onLogout}
            className="px-3 py-1 bg-red-500 text-white rounded hover:bg-red-600 transition text-sm"
          >
            Logout
          </button>
        </div>

      </div>
    

      {/* Content */}
      {currentPage === 'home' && <HomePage setCurrentPage={setCurrentPage} />}
      {currentPage === 'setup' && <SetupPage />}
      {currentPage === 'pedestrians' && <PedestriansPage />}
      {currentPage === 'monitor' && <MonitorPage />}

      {/* Footer */}
      <div className="fixed bottom-4 right-4">
        <p className="text-sm text-gray-500">AllWays Safe</p>
      </div>
    </div>
  );
};

// ==========================
// HOME PAGE (DASHBOARD MENU)
// ==========================
const HomePage = ({ setCurrentPage }) => {
  const menuItems = [
    { id: 'setup', icon: <Settings size={48}  color="#696969"/>, label: 'Set Up' },
    { id: 'pedestrians', icon: <Users size={48} color="#800000" />, label: 'Pedestrians' },
    { id: 'monitor', icon: <Monitor size={48} color="#000000" />, label: 'Monitor' }
  ];

  return (
    <div className="flex items-center justify-center min-h-[80vh]">
      <div className="grid grid-cols-3 gap-16">
        {menuItems.map((item) => (
          <button
            key={item.id}
            onClick={() => setCurrentPage(item.id)}
            className="flex flex-col items-center justify-center w-60 h-60 bg-white rounded-lg shadow hover:shadow-lg transition"
          >
            <div className="mb-4">{item.icon}</div>
            <p className="text-lg font-medium">{item.label}</p>
          </button>
        ))}
      </div>
    </div>
  );
};

// ==========================
// OTHER PAGES PLACEHOLDERS
// ==========================

const SetupPage = () => {
  const [controlBoxes, setControlBoxes] = useState([]);
  const [trafficSemaphores, setTrafficSemaphores] = useState([]);
  const [pedestrianSemaphores, setPedestrianSemaphores] = useState([]);
  const [selectedTab, setSelectedTab] = useState('controlbox');
  const [editingItem, setEditingItem] = useState(null);

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    const { data: cbData } = await supabase
      .from('controlbox')
      .select('*');

    const { data: tsData } = await supabase
      .from('t_semaphore')
      .select(`
        *,
        destinations ( destinationnumber )
      `);

    const { data: psData } = await supabase
      .from('p_semaphore')
      .select('*');

    const parsedTraffic = (tsData || []).map((t) => ({
      ...t,
      destinations: t.destinations
        ? t.destinations.map((d) => d.destinationnumber)
        : []
    }));

    setControlBoxes(cbData || []);
    setTrafficSemaphores(parsedTraffic);
    setPedestrianSemaphores(psData || []);
  };

  const saveControlBox = async (data) => {
    if (editingItem) {
      await supabase.from('controlbox').update(data).eq('id', editingItem.id);
    } else {
      await supabase.from('controlbox').insert([data]);
    }
    loadData();
    setEditingItem(null);
  };

  const saveTrafficSemaphore = async (data) => {
    if (data._delete) {
      await supabase
        .from('t_semaphore')
        .delete()
        .eq('id', data.id);

      setEditingItem(null);
      loadData();
      return;
    }

    const { destinations, ...tSemaphoreData } = data;

    let tSemaphoreId = editingItem?.id;

    if (editingItem) {
      const { error } = await supabase
        .from('t_semaphore')
        .update(tSemaphoreData)
        .eq('id', editingItem.id);

      if (error) {
        console.error('Update t_semaphore failed:', error);
        return;
      }
    }
    else {
      const { data: inserted, error } = await supabase
        .from('t_semaphore')
        .insert([tSemaphoreData])
        .select('id')
        .single();

      if (error) {
        console.error('Insert t_semaphore failed:', error);
        return;
      }

      tSemaphoreId = inserted.id;
    }

    await supabase
      .from('destinations')
      .delete()
      .eq('tsemid', tSemaphoreId);

    if (destinations?.length) {
      const payload = destinations.map((d) => ({
        tsemid: tSemaphoreId,
        destinationnumber: d
      }));

      const { error } = await supabase
        .from('destinations')
        .insert(payload);

      if (error) {
        console.error('Insert destinations failed:', error);
        return;
      }
    }

    setEditingItem(null);
    loadData();
  };



  const savePedestrianSemaphore = async (data) => {
    if (data._delete) {
      await supabase
        .from('p_semaphore')
        .delete()
        .eq('id', data.id);

      setEditingItem(null);
      loadData();
      return;
    }

    if (editingItem) {
      await supabase
        .from('p_semaphore')
        .update(data)
        .eq('id', editingItem.id);
    } else {
      await supabase.from('p_semaphore').insert([data]);
    }

    loadData();
    setEditingItem(null);
  };

  return (
    <div className="p-8">
      <div className="flex gap-4 mb-6 border-b">
        <button
          onClick={() => setSelectedTab('controlbox')}
          className={`px-6 py-3 font-medium ${
            selectedTab === 'controlbox' ? 'border-b-2 border-blue-600 text-blue-600' : 'text-gray-600'
          }`}
        >
          Control Box ⓘ
        </button>
        <button
          onClick={() => setSelectedTab('traffic')}
          className={`px-6 py-3 font-medium ${
            selectedTab === 'traffic' ? 'border-b-2 border-blue-600 text-blue-600' : 'text-gray-600'
          }`}
        >
          Traffic Semaphores ⓘ
        </button>
        <button
          onClick={() => setSelectedTab('pedestrian')}
          className={`px-6 py-3 font-medium ${
            selectedTab === 'pedestrian' ? 'border-b-2 border-blue-600 text-blue-600' : 'text-gray-600'
          }`}
        >
          Pedestrian Semaphores ⓘ
        </button>
      </div>

      {selectedTab === 'controlbox' && (
        <ControlBoxForm 
          items={controlBoxes} 
          onSave={saveControlBox}
          editingItem={editingItem}
          setEditingItem={setEditingItem}
        />
      )}
      {selectedTab === 'traffic' && (
        <TrafficSemaphoreForm 
          items={trafficSemaphores}
          controlBoxes={controlBoxes}
          onSave={saveTrafficSemaphore}
          editingItem={editingItem}
          setEditingItem={setEditingItem}
        />
      )}
      {selectedTab === 'pedestrian' && (
        <PedestrianSemaphoreForm 
          items={pedestrianSemaphores}
          controlBoxes={controlBoxes}
          onSave={savePedestrianSemaphore}
          editingItem={editingItem}
          setEditingItem={setEditingItem}
        />
      )}
    </div>
  );
};


const ControlBoxForm = () => {
  const [controlBoxes, setControlBoxes] = useState([]);
  const [tmcs, setTmcs] = useState([]);
  const [editingItem, setEditingItem] = useState(null);
  const [formData, setFormData] = useState({
    password: '',
    name: '',
    location: '',
    tmcid: '',
    ssid: '',
    ssid_password: ''
  });
  const [showPassword, setShowPassword] = useState(false);
  const [showSSIDPassword, setShowSSIDPassword] = useState(false);

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    const { data: cbData } = await supabase.from('controlbox').select('*');
    const { data: tmcData } = await supabase.from('tmc').select('*');
    setControlBoxes(cbData || []);
    setTmcs(tmcData || []);
  };

  const handleSubmit = async (e) => {
    e.preventDefault();

    try {
      if (editingItem) {
        await supabase.from('controlbox').update({
          password: formData.password,
          name: formData.name,
          location: formData.location,
          tmcid: formData.tmcid,
          ssid: formData.ssid,
          ssid_password: formData.ssid_password
        }).eq('id', editingItem.id);
      } else {
        await supabase.from('controlbox').insert([{
          password: formData.password,
          name: formData.name,
          location: formData.location,
          tmcid: formData.tmcid,
          ssid: formData.ssid,
          ssid_password: formData.ssid_password
        }]);
      }

      setEditingItem(null);
      setFormData({
        password: '',
        name: '',
        location: '',
        tmcid: '',
        ssid: '',
        ssid_password: ''
      });
      setShowPassword(false);
      setShowSSIDPassword(false);
      loadData();
    } catch (err) {
      console.error('Error creating/updating Control Box:', err);
    }
  };

  const handleDelete = async (id) => {
    if (!window.confirm('Are you sure you want to delete this Control Box?')) return;
    await supabase.from('controlbox').delete().eq('id', id);
    loadData();
  };

  useEffect(() => {
    if (editingItem) {
      setFormData({
        password: editingItem.password || '',
        name: editingItem.name || '',
        location: editingItem.location || '',
        tmcid: editingItem.tmcid || '',
        ssid: editingItem.ssid || '',
        ssid_password: editingItem.ssid_password || ''
      });
    }
  }, [editingItem]);

  return (
    <div className="grid grid-cols-2 gap-8">
      {/* FORM */}
      <div>
        <h3 className="text-xl font-semibold mb-4">
          {editingItem ? 'Edit Control Box' : 'Add New Control Box'}
        </h3>

        <form onSubmit={handleSubmit} className="space-y-4 bg-white p-6 rounded-xl shadow">

          {/* Password */}
          <div className="relative">
            <input
              type={showPassword ? 'text' : 'password'}
              placeholder="Password"
              value={formData.password}
              onChange={(e) => setFormData({ ...formData, password: e.target.value })}
              className="w-full p-3 border rounded-lg pr-10"
            />
            <button
              type="button"
              onClick={() => setShowPassword(!showPassword)}
              className="absolute top-1/2 right-3 -translate-y-1/2 text-gray-500"
            >
              {showPassword ? <EyeOff size={20} /> : <Eye size={20} />}
            </button>
          </div>

          <input
            type="text"
            placeholder="Name"
            value={formData.name}
            onChange={(e) => setFormData({ ...formData, name: e.target.value })}
            className="w-full p-3 border rounded-lg"
            required
          />

          <input
            type="text"
            placeholder="Location"
            value={formData.location}
            onChange={(e) => setFormData({ ...formData, location: e.target.value })}
            className="w-full p-3 border rounded-lg"
          />

          {/* TMC Dropdown */}
          <select
            value={formData.tmcid}
            onChange={(e) => setFormData({ ...formData, tmcid: e.target.value })}
            className="w-full p-3 border rounded-lg"
            required
          >
            <option value="">Select TMC</option>
            {tmcs.map((t) => (
              <option key={t.id} value={t.id}>
                {t.username} - {t.name || 'TMC'}
              </option>
            ))}
          </select>

          {/* SSID */}
          <input
            type="text"
            placeholder="SSID"
            value={formData.ssid}
            onChange={(e) => setFormData({ ...formData, ssid: e.target.value })}
            className="w-full p-3 border rounded-lg"
          />

          {/* SSID Password */}
          <div className="relative">
            <input
              type={showSSIDPassword ? 'text' : 'password'}
              placeholder="SSID Password"
              value={formData.ssid_password}
              onChange={(e) => setFormData({ ...formData, ssid_password: e.target.value })}
              className="w-full p-3 border rounded-lg pr-10"
            />
            <button
              type="button"
              onClick={() => setShowSSIDPassword(!showSSIDPassword)}
              className="absolute top-1/2 right-3 -translate-y-1/2 text-gray-500"
            >
              {showSSIDPassword ? <EyeOff size={20} /> : <Eye size={20} />}
            </button>
          </div>

          {/* Submit */}
          <button
            type="submit"
            className="w-full p-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
          >
            {editingItem ? 'Update' : 'Add'} Control Box
          </button>

          {editingItem && (
            <button
              type="button"
              onClick={() => {
                setEditingItem(null);
                setFormData({
                  password: '',
                  name: '',
                  location: '',
                  tmcid: '',
                  ssid: '',
                  ssid_password: ''
                });
                setShowPassword(false);
                setShowSSIDPassword(false);
              }}
              className="w-full p-3 bg-gray-300 rounded-lg hover:bg-gray-400 mt-2"
            >
              Cancel
            </button>
          )}
        </form>
      </div>

      {/* EXISTING CONTROL BOXES */}
      <div>
        <h3 className="text-xl font-semibold mb-4">Existing Control Boxes</h3>
        <div className="space-y-4 max-h-[600px] overflow-y-auto">
          {controlBoxes.map((cb) => (
            <div key={cb.id} className="p-4 bg-white border rounded-lg flex justify-between items-start">
              <div>
                <p className="font-semibold">{cb.name || cb.id}</p>
                <p className="text-sm text-gray-600">Location: {cb.location}</p>
                <p className="text-sm text-gray-600">TMC: {cb.tmcid}</p>
                <p className="text-sm text-gray-600">SSID: {cb.ssid}</p>
              </div>
              <div className="flex gap-2">
                <button
                  onClick={() => setEditingItem(cb)}
                  className="text-blue-600 hover:text-blue-800"
                >
                  <Pencil size={18} />
                </button>
                <button
                  onClick={() => handleDelete(cb.id)}
                  className="text-red-600 hover:text-red-800"
                >
                  <Trash2 size={18} />
                </button>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

const TrafficSemaphoreForm = ({
  items,
  controlBoxes,
  onSave,
  editingItem,
  setEditingItem
}) => {
  const [formData, setFormData] = useState({
    name: '',
    location: '',
    controlboxid: '',
    gpio_red: '',
    gpio_yellow: '',
    gpio_green: '',
    destinations: []
  });

  const [destinationInput, setDestinationInput] = useState('');

  useEffect(() => {
    if (editingItem) {
      setFormData({
        name: editingItem.name || '',
        location: editingItem.location || '',
        controlboxid: editingItem.controlboxid || '',
        gpio_red: editingItem.gpio_red ?? '',
        gpio_yellow: editingItem.gpio_yellow ?? '',
        gpio_green: editingItem.gpio_green ?? '',
        destinations: editingItem.destinations || []
      });
    }
  }, [editingItem]);

  const resetForm = () => {
    setEditingItem(null);
    setFormData({
      name: '',
      location: '',
      controlboxid: '',
      gpio_red: '',
      gpio_yellow: '',
      gpio_green: '',
      destinations: []
    });
    setDestinationInput('');
  };

  const addDestination = () => {
    if (destinationInput === '') return;

    setFormData((prev) => ({
      ...prev,
      destinations: [...prev.destinations, Number(destinationInput)]
    }));

    setDestinationInput('');
  };

  const removeDestination = (index) => {
    setFormData((prev) => ({
      ...prev,
      destinations: prev.destinations.filter((_, i) => i !== index)
    }));
  };

  const handleSubmit = (e) => {
    e.preventDefault();

    const payload = {
      name: formData.name,
      location: formData.location,
      controlboxid: formData.controlboxid,
      gpio_red: Number(formData.gpio_red),
      gpio_yellow: Number(formData.gpio_yellow),
      gpio_green: Number(formData.gpio_green),
      status: null,
      destinations: formData.destinations
    };

    onSave(payload);
    resetForm();
  };

  return (
    <div className="grid grid-cols-2 gap-8">

      {/* ================= FORM ================= */}
      <div>
        <h3 className="text-xl font-semibold mb-4">
          {editingItem ? 'Edit Traffic Semaphore' : 'Add Traffic Semaphore'}
        </h3>

        <form
          onSubmit={handleSubmit}
          className="space-y-4 bg-white p-6 rounded-xl shadow"
        >
          <input
            type="text"
            placeholder="Name"
            value={formData.name}
            onChange={(e) =>
              setFormData({ ...formData, name: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          <input
            type="text"
            placeholder="Location"
            value={formData.location}
            onChange={(e) =>
              setFormData({ ...formData, location: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          {/* CONTROL BOX */}
          <select
            required
            value={formData.controlboxid}
            onChange={(e) =>
              setFormData({ ...formData, controlboxid: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          >
            <option value="">Select Control Box</option>
            {controlBoxes.map((cb) => (
              <option key={cb.id} value={cb.id}>
                {cb.name} - {cb.location}
              </option>
            ))}
          </select>

          {/* GPIOS */}
          <input
            type="number"
            min={0}
            max={40}
            placeholder="GPIO Red (0–40)"
            value={formData.gpio_red}
            onChange={(e) =>
              setFormData({ ...formData, gpio_red: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          <input
            type="number"
            min={0}
            max={40}
            placeholder="GPIO Yellow (0–40)"
            value={formData.gpio_yellow}
            onChange={(e) =>
              setFormData({ ...formData, gpio_yellow: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          <input
            type="number"
            min={0}
            max={40}
            placeholder="GPIO Green (0–40)"
            value={formData.gpio_green}
            onChange={(e) =>
              setFormData({ ...formData, gpio_green: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          {/* DESTINATIONS */}
          <div>
            <p className="font-medium mb-2">Destinations</p>

            <div className="flex gap-2 mb-2">
              <input
                type="number"
                placeholder="Destination number"
                value={destinationInput}
                onChange={(e) => setDestinationInput(e.target.value)}
                className="flex-1 p-2 border rounded-lg"
              />
              <button
                type="button"
                onClick={addDestination}
                className="p-2 bg-blue-600 text-white rounded-lg"
              >
                <Plus size={16} />
              </button>
            </div>

            <div className="space-y-2">
              {formData.destinations.map((d, i) => (
                <div
                  key={i}
                  className="flex justify-between items-center bg-gray-100 p-2 rounded-lg"
                >
                  <span>{d}</span>
                  <button
                    type="button"
                    onClick={() => removeDestination(i)}
                    className="text-red-600"
                  >
                    <Trash2 size={14} />
                  </button>
                </div>
              ))}
            </div>
          </div>

          <button
            type="submit"
            className="w-full p-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
          >
            {editingItem ? 'Update' : 'Add'} Traffic Semaphore
          </button>

          {editingItem && (
            <button
              type="button"
              onClick={resetForm}
              className="w-full p-3 bg-gray-300 rounded-lg hover:bg-gray-400"
            >
              Cancel
            </button>
          )}
        </form>
      </div>

      {/* ================= LIST ================= */}
      <div>
        <h3 className="text-xl font-semibold mb-4">
          Existing Traffic Semaphores
        </h3>

        <div className="space-y-4 max-h-[600px] overflow-y-auto">
          {items.map((item) => (
            <div
              key={item.id}
              className="p-4 bg-white border rounded-lg flex justify-between"
            >
              <div>
                <p className="font-semibold">{item.name}</p>
                <p className="text-sm text-gray-600">{item.location}</p>
                <p className="text-sm text-gray-600">
                  GPIO: R:{item.gpio_red} Y:{item.gpio_yellow} G:{item.gpio_green}
                </p>
                {item.destinations?.length > 0 && (
                  <p className="text-sm text-gray-600">
                    Destinations: {item.destinations.join(', ')}
                  </p>
                )}
              </div>

              <div className="flex gap-2">
                <button
                  onClick={() => setEditingItem(item)}
                  className="text-blue-600 hover:text-blue-800"
                >
                  <Pencil size={18} />
                </button>

                <button
                  onClick={() => {
                    if (window.confirm('Delete this traffic semaphore?')) {
                      setEditingItem(null);
                      onSave({ id: item.id, _delete: true });
                    }
                  }}
                  className="text-red-600 hover:text-red-800"
                >
                  <Trash2 size={18} />
                </button>
              </div>
            </div>
          ))}
        </div>
      </div>

    </div>
  );
};


const PedestrianSemaphoreForm = ({
  items,
  controlBoxes,
  onSave,
  editingItem,
  setEditingItem
}) => {
  const [formData, setFormData] = useState({
    controlboxid: '',
    name: '',
    location: '',
    gpio_red: '',
    gpio_green: '',
    hasCardReader: 0,
    hasBuzzer: 0,
    hasButton: 0,
    gpio_button: '',
    buttonThreshold: ''
  });

  useEffect(() => {
    if (editingItem) {
      setFormData({
        controlboxid: editingItem.controlboxid || '',
        name: editingItem.name || '',
        location: editingItem.location || '',
        gpio_red: editingItem.gpio_red ?? '',
        gpio_green: editingItem.gpio_green ?? '',
        hasCardReader: editingItem.hasCardReader ?? 0,
        hasBuzzer: editingItem.hasBuzzer ?? 0,
        hasButton: editingItem.hasButton ?? 0,
        gpio_button: editingItem.gpio_button ?? '',
        buttonThreshold: editingItem.buttonThreshold ?? ''
      });
    }
  }, [editingItem]);

  const resetForm = () => {
    setEditingItem(null);
    setFormData({
      controlboxid: '',
      name: '',
      location: '',
      gpio_red: '',
      gpio_green: '',
      hasCardReader: 0,
      hasBuzzer: 0,
      hasButton: 0,
      gpio_button: '',
      buttonThreshold: ''
    });
  };

  const handleSubmit = (e) => {
    e.preventDefault();

    const payload = {
      controlboxid: formData.controlboxid,
      name: formData.name,
      location: formData.location,
      gpio_red: Number(formData.gpio_red),
      gpio_green: Number(formData.gpio_green),
      status: null,
      hasCardReader: Number(formData.hasCardReader),
      hasBuzzer: Number(formData.hasBuzzer),
      hasButton: Number(formData.hasButton),
      gpio_button: formData.hasButton ? Number(formData.gpio_button) : null,
      buttonThreshold: formData.hasButton ? Number(formData.buttonThreshold) : null
    };

    onSave(payload);
    resetForm();
  };

  const handleDelete = (item) => {
    if (window.confirm('Delete this pedestrian semaphore?')) {
      onSave({ id: item.id, _delete: true });
      resetForm();
    }
  };

  return (
    <div className="grid grid-cols-2 gap-8">

      {/* ================= FORM ================= */}
      <div>
        <h3 className="text-xl font-semibold mb-4">
          {editingItem ? 'Edit Pedestrian Semaphore' : 'Add Pedestrian Semaphore'}
        </h3>

        <form
          onSubmit={handleSubmit}
          className="space-y-4 bg-white p-6 rounded-xl shadow"
        >
          {/* CONTROL BOX */}
          <select
            required
            value={formData.controlboxid}
            onChange={(e) =>
              setFormData({ ...formData, controlboxid: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          >
            <option value="">Select Control Box</option>
            {controlBoxes.map((cb) => (
              <option key={cb.id} value={cb.id}>
                {cb.name} - {cb.location}
              </option>
            ))}
          </select>

          <input
            type="text"
            placeholder="Name"
            value={formData.name}
            onChange={(e) =>
              setFormData({ ...formData, name: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          <input
            type="text"
            placeholder="Location"
            value={formData.location}
            onChange={(e) =>
              setFormData({ ...formData, location: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          {/* GPIOS */}
          <input
            type="number"
            min={0}
            max={40}
            placeholder="GPIO Red (0–40)"
            value={formData.gpio_red}
            onChange={(e) =>
              setFormData({ ...formData, gpio_red: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          <input
            type="number"
            min={0}
            max={40}
            placeholder="GPIO Green (0–40)"
            value={formData.gpio_green}
            onChange={(e) =>
              setFormData({ ...formData, gpio_green: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          />

          {/* CARD READER */}
          <select
            value={formData.hasCardReader}
            onChange={(e) =>
              setFormData({ ...formData, hasCardReader: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          >
            <option value={0}>No Card Reader</option>
            <option value={1}>Has Card Reader</option>
          </select>

          {/* BUZZER */}
          <select
            value={formData.hasBuzzer}
            onChange={(e) =>
              setFormData({ ...formData, hasBuzzer: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          >
            <option value={0}>No Buzzer</option>
            <option value={1}>Has Buzzer</option>
          </select>

          {/* BUTTON */}
          <select
            value={formData.hasButton}
            onChange={(e) =>
              setFormData({ ...formData, hasButton: e.target.value })
            }
            className="w-full p-3 border rounded-lg"
          >
            <option value={0}>No Button</option>
            <option value={1}>Has Button</option>
          </select>

          {formData.hasButton == 1 && (
            <>
              <input
                type="number"
                min={0}
                max={40}
                placeholder="GPIO Button (0–40)"
                value={formData.gpio_button}
                onChange={(e) =>
                  setFormData({ ...formData, gpio_button: e.target.value })
                }
                className="w-full p-3 border rounded-lg"
              />

              <input
                type="number"
                placeholder="Button Threshold"
                value={formData.buttonThreshold}
                onChange={(e) =>
                  setFormData({ ...formData, buttonThreshold: e.target.value })
                }
                className="w-full p-3 border rounded-lg"
              />
            </>
          )}

          <button
            type="submit"
            className="w-full p-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
          >
            {editingItem ? 'Update' : 'Add'} Pedestrian Semaphore
          </button>

          {editingItem && (
            <button
              type="button"
              onClick={resetForm}
              className="w-full p-3 bg-gray-300 rounded-lg hover:bg-gray-400"
            >
              Cancel
            </button>
          )}
        </form>
      </div>

      {/* ================= LIST ================= */}
      <div>
        <h3 className="text-xl font-semibold mb-4">
          Existing Pedestrian Semaphores
        </h3>

        <div className="space-y-4 max-h-[600px] overflow-y-auto">
          {items.map((item) => (
            <div
              key={item.id}
              className="p-4 bg-white border rounded-lg flex justify-between items-start"
            >
              <div>
                <p className="font-semibold">{item.name || item.id}</p>
                <p className="text-sm text-gray-600">Location: {item.location}</p>
                <p className="text-sm text-gray-600">
                  GPIO: R:{item.gpio_red} G:{item.gpio_green} {item.hasButton ? `Button GPIO: ${item.gpio_button}, Threshold: ${item.buttonThreshold}` : ''}
                </p>
                <p className="text-sm text-gray-600">
                  Control Box: {item.controlboxid}
                </p>
                <p className="text-sm text-gray-600">
                  Card Reader: {item.hasCardReader ? 'Yes' : 'No'}, Buzzer: {item.hasBuzzer ? 'Yes' : 'No'}, Button: {item.hasButton ? 'Yes' : 'No'}
                </p>
              </div>

              <div className="flex gap-2">
                <button
                  onClick={() => setEditingItem(item)}
                  className="text-blue-600 hover:text-blue-800"
                >
                  <Pencil size={18} />
                </button>

                <button
                  onClick={() => handleDelete(item)}
                  className="text-red-600 hover:text-red-800"
                >
                  <Trash2 size={18} />
                </button>
              </div>
            </div>
          ))}
        </div>
      </div>

    </div>
  );
};


const PedestriansPage = () => {
  const [pedestrians, setPedestrians] = useState([]);
  const [searchTerm, setSearchTerm] = useState('');
  const [editingItem, setEditingItem] = useState(null);
  const [formData, setFormData] = useState({
    cc_id: '',
    name: '',
    physicaltag_id: '',
    disability: ''
  });

  useEffect(() => {
    loadData();

    const channel = supabase
      .channel('pedestrian-changes')
      .on(
        'postgres_changes',
        { event: '*', schema: 'public', table: 'pedestrian' },
        loadData
      )
      .subscribe();

    return () => {
      supabase.removeChannel(channel);
    };
  }, []);

  const loadData = async () => {
    const { data } = await supabase.from('pedestrian').select('*');
    setPedestrians(data || []);
  };

  const handleSubmit = async (e) => {
    e.preventDefault();

    const payload = {
      ...formData,
      disability: parseInt(formData.disability)
    };

    if (editingItem) {
      const { data, error } = await supabase
        .from('pedestrian')
        .update(payload)
        .eq('cc_id', editingItem.cc_id)
        .select()
        .single();

      if (!error && data) {
        setPedestrians((prev) =>
          prev.map((p) => (p.cc_id === data.cc_id ? data : p))
        );
      }
    } else {
      const { data, error } = await supabase
        .from('pedestrian')
        .insert([payload])
        .select()
        .single();

      if (!error && data) {
        setPedestrians((prev) => [data, ...prev]);
      }
    }

    setEditingItem(null);
    setFormData({
      cc_id: '',
      name: '',
      physicaltag_id: '',
      disability: ''
    });
  };

  const handleDelete = async (cc_id) => {
    const confirmDelete = window.confirm(
      'Are you sure you want to delete this pedestrian?'
    );
    if (!confirmDelete) return;

    const { error } = await supabase
      .from('pedestrian')
      .delete()
      .eq('cc_id', cc_id);

    if (!error) {
      setPedestrians((prev) =>
        prev.filter((p) => p.cc_id !== cc_id)
      );
    } else {
      console.error(error);
      alert(error.message);
    }
  };

  const filteredPedestrians = pedestrians.filter(
    (p) =>
      p.cc_id.includes(searchTerm) ||
      (p.physicaltag_id && p.physicaltag_id.includes(searchTerm))
  );

  return (
    <div className="p-8">
      <div className="grid grid-cols-2 gap-8">
        <div>
          <h3 className="text-xl font-semibold mb-4">
            {editingItem ? 'Edit Pedestrian' : 'Add New Pedestrian'}
          </h3>

          <form
            className="space-y-4 bg-white p-6 rounded-xl shadow"
            onSubmit={handleSubmit}
          >
            <input
              type="text"
              placeholder="CC ID"
              value={formData.cc_id}
              onChange={(e) =>
                setFormData({ ...formData, cc_id: e.target.value })
              }
              className="w-full p-3 border rounded-lg"
              required
              disabled={!!editingItem}
            />

            <input
              type="text"
              placeholder="Name"
              value={formData.name}
              onChange={(e) =>
                setFormData({ ...formData, name: e.target.value })
              }
              className="w-full p-3 border rounded-lg"
            />

            <input
              type="text"
              placeholder="Physical Tag ID"
              value={formData.physicaltag_id}
              onChange={(e) =>
                setFormData({
                  ...formData,
                  physicaltag_id: e.target.value
                })
              }
              className="w-full p-3 border rounded-lg"
            />

            <input
              type="number"
              placeholder="Disability Level"
              value={formData.disability}
              onChange={(e) =>
                setFormData({
                  ...formData,
                  disability: e.target.value
                })
              }
              className="w-full p-3 border rounded-lg"
            />

            <button
              type="submit"
              className="w-full p-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
            >
              {editingItem ? 'Update' : 'Add'} Pedestrian
            </button>
          </form>
        </div>

        {/* RIGHT – REGISTERED */}
        <div>
          <h3 className="text-xl font-semibold mb-4">
            Registered Pedestrians
          </h3>

          <input
            type="text"
            placeholder="Search by CC_ID or Tag"
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full p-3 border rounded-lg mb-4"
          />

          <div className="space-y-4 max-h-[600px] overflow-y-auto">
            {filteredPedestrians.map((ped) => (
              <div
                key={ped.cc_id}
                className="p-4 bg-white border rounded-xl shadow-sm"
              >
                <div className="flex justify-between items-start">
                  <div>
                    <p className="font-semibold">{ped.cc_id}</p>
                    <p className="text-sm text-gray-600">
                      Name: {ped.name || 'N/A'}
                    </p>
                    <p className="text-sm text-gray-600">
                      Physical Tag: {ped.physicaltag_id || 'N/A'}
                    </p>
                    <p className="text-sm text-gray-600">
                      Disability: {ped.disability ?? 'N/A'}
                    </p>
                  </div>

                  <div className="flex gap-3">
                    <button
                      onClick={() => {
                        setEditingItem(ped);
                        setFormData(ped);
                      }}
                      className="text-blue-600 hover:text-blue-800"
                    >
                      <Pencil size={16} />
                    </button>

                    <button
                      onClick={() => handleDelete(ped.cc_id)}
                      className="text-red-600 hover:text-red-800"
                    >
                      <Trash2 size={16} />
                    </button>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>

      </div>
    </div>
  );
};


const MonitorPage = () => {
  const [controlBoxes, setControlBoxes] = useState([]);
  const [evRecords, setEvRecords] = useState([]);
  const [pedestrianRecords, setPedestrianRecords] = useState([]);
  const [evSearch, setEvSearch] = useState('');
  const [pedSearch, setPedSearch] = useState('');
  const [cbSearch, setCbSearch] = useState(''); 

  const loadData = async () => {
    const { data: cbData } = await supabase
      .from('controlbox')
      .select(`*, t_semaphore(*), p_semaphore(*)`);

    const { data: evData } = await supabase
      .from('emergencyvehicle')
      .select('*, controlbox!inner(name)')
      .order('timestamp', { ascending: false })
      .limit(50);

    const { data: pedData } = await supabase
      .from('p_semaphore_pedestrian')
      .select('*, p_semaphore!inner(name)')
      .order('timestamp', { ascending: false })
      .limit(50);

    setControlBoxes(cbData || []);
    setEvRecords(evData || []);
    setPedestrianRecords(pedData || []);
  };

  const updateTrafficSemaphore = (updated) => {
    setControlBoxes((prev) =>
      prev.map((cb) => ({
        ...cb,
        t_semaphore: cb.t_semaphore?.map((ts) =>
          ts.id === updated.id ? { ...ts, ...updated } : ts
        ),
      }))
    );
  };

  const updatePedestrianSemaphore = (updated) => {
    setControlBoxes((prev) =>
      prev.map((cb) => ({
        ...cb,
        p_semaphore: cb.p_semaphore?.map((ps) =>
          ps.id === updated.id ? { ...ps, ...updated } : ps
        ),
      }))
    );
  };

  const updateEmergencyVehicle = (newEV) => {
    setEvRecords((prev) => [newEV, ...prev].slice(0, 50));
  };

  useEffect(() => {
    loadData();

    const channel = supabase
      .channel('monitor-realtime')
      .on('postgres_changes', { event: '*', schema: 'public', table: 't_semaphore' }, (payload) =>
        updateTrafficSemaphore(payload.new)
      )
      .on('postgres_changes', { event: '*', schema: 'public', table: 'p_semaphore' }, (payload) =>
        updatePedestrianSemaphore(payload.new)
      )
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'emergencyvehicle' }, (payload) =>
        updateEmergencyVehicle(payload.new)
      )
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'p_semaphore_pedestrian' }, () =>
        loadData()
      )
      .subscribe();

    return () => supabase.removeChannel(channel);
  }, []);

  const getStatusColor = (status) => {
    if (!status) return 'bg-gray-400';
    switch (status.toString().toLowerCase()) {
      case 'red':
        return 'bg-red-500';
      case 'green':
        return 'bg-green-500';
      case 'yellow':
        return 'bg-yellow-400';
      default:
        return 'bg-gray-400';
    }
  };

  const getPriorityColor = (priority) => {
    switch (priority) {
      case 1:
        return 'bg-red-100 text-red-800 border-red-300';
      case 2:
        return 'bg-orange-100 text-orange-800 border-orange-300';
      case 3:
        return 'bg-yellow-100 text-yellow-800 border-yellow-300';
      default:
        return 'bg-gray-100 text-gray-800 border-gray-300';
    }
  };

  const filteredEvRecords = evRecords.filter(ev =>
    ev.licenseplate?.toLowerCase().includes(evSearch.toLowerCase())
  );

  const filteredPedRecords = pedestrianRecords.filter(pr =>
    pr.pedestrianCC_id?.includes(pedSearch)
  );

  const filteredControlBoxes = controlBoxes.filter(cb =>
    cb.name?.toLowerCase().includes(cbSearch.toLowerCase())
  );

  return (
    <div className="p-8 bg-gray-100 min-h-screen">
      <div className="grid grid-cols-3 gap-8">

        {/* CONTROL BOX STATUS */}
        <div>
          <h3 className="text-2xl font-bold mb-4 text-gray-800 flex items-center gap-2">
            <SquareChartGantt size={24} /> Control Box Status
          </h3>

          {/* SEARCH FOR CONTROLBOX */}
          <div className="mb-4 flex items-center gap-2 bg-white rounded-xl shadow p-2 border border-gray-200">
            <Search size={20} className="text-gray-400" />
            <input
              type="text"
              placeholder="Search Control Box"
              value={cbSearch}
              onChange={(e) => setCbSearch(e.target.value)}
              className="flex-1 p-2 focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg"
            />
          </div>

          <div className="space-y-4">
            {controlBoxes.map((cb) => (
              <div key={cb.id} className="bg-white rounded-2xl shadow-md p-6 border border-gray-200 hover:shadow-xl transition">
                <div className="mb-4 pb-3 border-b border-gray-200">
                  <p className="text-lg font-bold text-gray-800">{cb.name || cb.id}</p>
                  <p className="text-sm text-gray-500 mt-1 flex items-center gap-1">
                    <MapPinned size={16} /> {cb.location}
                  </p>
                </div>

                {/* TRAFFIC SEMAPHORES */}
                <div className="mb-4">
                  <p className="text-xs font-semibold text-gray-500 uppercase mb-2">Traffic Semaphores</p>
                  <div className="space-y-2">
                    {cb.t_semaphore?.map((ts) => (
                      <div key={ts.id} className="flex items-center justify-between bg-gray-50 rounded-xl p-2 shadow-sm">
                        <span className="flex items-center gap-2 text-sm font-medium text-gray-700">
                           {ts.name || ts.id}
                        </span>
                        <div className="flex items-center gap-2">
                          <span className={`w-3 h-3 rounded-full ${getStatusColor(ts.status)}`} />
                          <span className="text-xs font-semibold text-gray-600 capitalize">{ts.status || 'unknown'}</span>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>

                {/* PEDESTRIAN SEMAPHORES */}
                <div>
                  <p className="text-xs font-semibold text-gray-500 uppercase mb-2">Pedestrian Semaphores</p>
                  <div className="space-y-2">
                    {cb.p_semaphore?.map((ps) => (
                      <div key={ps.id} className="flex items-center justify-between bg-gray-50 rounded-xl p-2 shadow-sm">
                        <span className="flex items-center gap-2 text-sm font-medium text-gray-700">
                          {ps.name || ps.id}
                        </span>
                        <div className="flex items-center gap-2">
                          <span className={`w-3 h-3 rounded-full ${getStatusColor(ps.status)}`} />
                          <span className="text-xs font-semibold text-gray-600 capitalize">{ps.status || 'unknown'}</span>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* EMERGENCY VEHICLES */}
        <div>
          <h3 className="text-2xl font-bold mb-4 text-gray-800 flex items-center gap-2">
            <Ambulance size={24} /> Emergency Vehicles
          </h3>

          {/* SEARCH FOR EV */}
          <div className="mb-4 flex items-center gap-2 bg-white rounded-xl shadow p-2 border border-gray-200">
            <Search size={20} className="text-gray-400" />
            <input
              type="text"
              placeholder="Search by License Plate"
              value={evSearch}
              onChange={(e) => setEvSearch(e.target.value)}
              className="flex-1 p-2 focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg"
            />
          </div>

          <div className="space-y-3 max-h-[600px] overflow-y-auto pr-2">
            {filteredEvRecords.map((ev) => (
              <div key={ev.id} className="bg-white rounded-2xl shadow-md p-5 border border-gray-200 hover:shadow-xl transition">
                <div className="flex items-center justify-between mb-3">
                  <p className="text-lg font-bold text-gray-800"><Ambulance size={16} /> {ev.licenseplate}</p>
                  <span className={`px-3 py-1 rounded-full text-xs font-semibold border ${getPriorityColor(ev.priority_level)}`}>
                    Priority {ev.priority_level || 'N/A'}
                  </span>
                </div>
                <div className="space-y-1 text-sm text-gray-600">
                  <p><strong>From:</strong> {ev.origin}</p>
                  <p><strong>To:</strong> {ev.destination}</p>
                  <p><strong>At:</strong> {ev.controlbox?.name || ev.controlbox_id}</p>
                  <p className="text-xs text-gray-500 mt-2">
                    <Clock size={16} /> {new Date(ev.timestamp).toLocaleString()}
                  </p>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* PEDESTRIAN ACTIVITY */}
        <div>
          <h3 className="text-2xl font-bold mb-4 text-gray-800 flex items-center gap-2">
            <IdCard size={24} /> Pedestrian Activity
          </h3>

          {/* SEARCH FOR PEDESTRIAN */}
          <div className="mb-4 flex items-center gap-2 bg-white rounded-xl shadow p-2 border border-gray-200">
            <Search size={20} className="text-gray-400" />
            <input
              type="text"
              placeholder="Search by CC_ID"
              value={pedSearch}
              onChange={(e) => setPedSearch(e.target.value)}
              className="flex-1 p-2 focus:outline-none focus:ring-2 focus:ring-blue-500 rounded-lg"
            />
          </div>

          <div className="space-y-3 max-h-[600px] overflow-y-auto pr-2">
            {filteredPedRecords.map((pr) => (
              <div key={pr.id} className="bg-white rounded-2xl shadow-md p-5 border border-gray-200 hover:shadow-xl transition">
                <p className="text-lg font-bold text-gray-800 mb-2"><IdCard size={16} /> {pr.pedestrianCC_id}</p>
                <div className="text-sm text-gray-600">
                  <p><strong>Semaphore:</strong> {pr.p_semaphore?.name || pr.psem_id}</p>
                  <p className="text-xs text-gray-500 mt-2">
                    <Clock size={16} /> {new Date(pr.timestamp).toLocaleString()}
                  </p>
                </div>
              </div>
            ))}
          </div>
        </div>

      </div>
    </div>
  );
};




// ==========================
// APP
// ==========================
export default function App() {
  const [user, setUser] = useState(null);

  return (
    <>
      {!user ? (
        <LoginPage onLogin={setUser} />
      ) : (
        <Dashboard onLogout={() => setUser(null)} />
      )}
    </>
  );
}