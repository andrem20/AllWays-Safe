const express = require('express');
const { createClient } = require('@supabase/supabase-js');
require('dotenv').config();

const app = express();
app.use(express.json());

// Initialize Supabase client
const supabase = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_KEY
);


app.use("/", require("./routes/post_rasp")(supabase));
app.use("/", require("./routes/patch_rasp")(supabase));
app.use("/", require("./routes/getList_rasp")(supabase));
app.use("/", require("./routes/getID_rasp")(supabase));


// SERVER STARTUP
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`TMC API server running on port ${PORT}`);
  console.log(`TMC endpoints:`);
  console.log(` POST `);
  console.log(` PATCH `);
  console.log(` GET LIST  `);
  console.log(` GET ID  `);
});

// Error handling middleware
app.use((err, req, res, next) => {
  console.error('Unhandled error:', err);
  res.status(500).json({
    id: 500,
    error: 500,
    detail: 'Internal server error'
  });
});
