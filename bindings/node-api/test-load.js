console.log('Step 1: Loading native addon...');
try {
    const addon = require('./build/Release/gamecalls_engine.node');
    console.log('Step 2: Addon loaded successfully');
    console.log('Step 3: Available functions:', Object.keys(addon));
} catch (error) {
    console.error('ERROR:', error.message);
    console.error(error.stack);
}
