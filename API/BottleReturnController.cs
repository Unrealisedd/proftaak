using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using BottleReturnApi.Data;
using BottleReturnApi.Models;
using System.ComponentModel.DataAnnotations.Schema;
using Microsoft.Extensions.Logging.Abstractions;

namespace BottleReturnApi.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class BottleReturnController(BottleReturnDbContext context) : ControllerBase
    {
        private readonly BottleReturnDbContext _context = context;

        [Table("donatie", Schema = "dbo")]
        public class BottleReturnRecord
        {
            public int ID { get; set; }
            public string? barcode { get; set; }
            public DateTime datum { get; set; }
            public decimal bedrag { get; set; }
            public bool? servoactivated { get; set; }
            public string? donateur_ID { get; set; }
            public string? scanner_ID { get; set; }
        }

        public class BottleReturnRequest
        {
            public string barcode { get; set; }
            public bool servoactivated { get; set; }
            public string scanner_ID { get; set; }
        }

        public class MobileBottleReturnRequest
        {
            public string barcode { get; set; }
            public int donateur_ID { get; set; }
        }

        public class LoginRequest
        {
            public string email { get; set; }
            public string password { get; set; }
        }

        public class SignupRequest
        {
            public string? naam { get; set; }
            public string? achternaam { get; set; }
            public string? volledige_naam { get; set; }
            public string wachtwoord { get; set; }
            public string email { get; set; }
        }

        [HttpPost("signup")]
public async Task<IActionResult> Signup([FromBody] SignupRequest request)
{
    if (string.IsNullOrEmpty(request.wachtwoord) || string.IsNullOrEmpty(request.email))
    {
        return BadRequest("Email and password are required");
    }

    // Get the highest existing ID and add 1
    var nextId = 1;
    if (await _context.Donateurs.AnyAsync())
    {
        nextId = await _context.Donateurs.MaxAsync(d => d.ID) + 1;
    }

    string fullName;
    if (!string.IsNullOrEmpty(request.volledige_naam))
    {
        fullName = request.volledige_naam;
    }
    else if (!string.IsNullOrEmpty(request.naam) && !string.IsNullOrEmpty(request.achternaam))
    {
        fullName = $"{request.naam} {request.achternaam}";
    }
    else
    {
        return BadRequest("Either provide volledige_naam or both naam and achternaam");
    }

    var newDonateur = new Donateur
    {
        ID = nextId,
        volledige_naam = fullName,
        wachtwoord = request.wachtwoord,
        email = request.email
    };

    _context.Add(newDonateur);
    await _context.SaveChangesAsync();

    return Ok(new { 
        message = "Registration successful",
        ID = newDonateur.ID 
    });
}

        [HttpGet("totaal")]
        public async Task<IActionResult> GetTotalDonations()
        {
            var totalAmount = await _context.Set<BottleReturnRecord>()
                .SumAsync(d => d.bedrag);

            return Ok(new { 
                total = $"€{totalAmount:F2}",
                amount = totalAmount
            });
        }

        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            var donateur = await _context.Set<Donateur>()
                .FirstOrDefaultAsync(d => d.email == request.email && d.wachtwoord == request.password);

            if (donateur == null)
            {
                return Unauthorized("Invalid credentials");
            }

            return Ok(new { ID = donateur.ID });
        }

        [HttpPost("process")]
        public async Task<IActionResult> ProcessBottleReturn([FromBody] BottleReturnRequest request)
        {
            try
            {
                if (request == null)
                {
                    await LogAsync("Error", "Null request received");
                    return BadRequest("Invalid return data");
                }

                var scanner = await _context.Set<Scanner>()
                    .FirstOrDefaultAsync(s => s.ID == int.Parse(request.scanner_ID));

                if (scanner == null)
                {
                    await LogAsync("Warning", $"Scanner not found: {request.scanner_ID}");
                    return NotFound($"Scanner ID {request.scanner_ID} not found in system");
                }

                var barcodeValue = await _context.Set<BarcodeValue>()
                    .FirstOrDefaultAsync(b => b.barcode == request.barcode);

                if (barcodeValue == null)
                {
                    await LogAsync("Warning", $"Barcode not found: {request.barcode}");
                    return NotFound($"Barcode {request.barcode} not found in system");
                }

                var record = new BottleReturnRecord
                {
                    barcode = request.barcode,
                    datum = DateTime.UtcNow,
                    bedrag = barcodeValue.bedrag,
                    servoactivated = request.servoactivated,
                    scanner_ID = request.scanner_ID,
                    donateur_ID = null
                };
               
                _context.Add(record);
                await _context.SaveChangesAsync();
                await LogAsync("Information", "Processed bottle return", null, record.ID.ToString());

                return Ok(new {
                    message = "Bottle return processed successfully",
                    id = record.ID,
                    amount = record.bedrag,
                    barcode = record.barcode
                });
            }
            catch (Exception ex)
            {
                await LogAsync("Error", "Processing failed", ex.ToString());
                throw;
            }
        }

        [HttpPost("mobile")]
        public async Task<IActionResult> ProcessMobileBottleReturn([FromBody] MobileBottleReturnRequest request)
        {
            try
            {
                if (request == null)
                {
                    await LogAsync("Error", "Null mobile request received");
                    return BadRequest("Invalid return data");
                }

                var donateur = await _context.Set<Donateur>()
                    .FirstOrDefaultAsync(d => d.ID == request.donateur_ID);

                if (donateur == null)
                {
                    await LogAsync("Warning", $"Donateur not found: {request.donateur_ID}");
                    return NotFound($"Donateur ID {request.donateur_ID} not found in system");
                }

                var barcodeValue = await _context.Set<BarcodeValue>()
                    .FirstOrDefaultAsync(b => b.barcode == request.barcode);

                if (barcodeValue == null)
                {
                    await LogAsync("Warning", $"Barcode not found: {request.barcode}");
                    return NotFound($"Barcode {request.barcode} not found in system");
                }

                var record = new BottleReturnRecord
                {
                    barcode = request.barcode,
                    datum = DateTime.UtcNow,
                    bedrag = barcodeValue.bedrag,
                    donateur_ID = request.donateur_ID.ToString(),
                    servoactivated = null,
                    scanner_ID = null
                };
               
                _context.Add(record);
                await _context.SaveChangesAsync();
                await LogAsync("Information", "Processed mobile bottle return", null, record.ID.ToString());

                return Ok($"Thank you, {donateur.volledige_naam} for donating {record.bedrag:C}!");
            }
            catch (Exception ex)
            {
                await LogAsync("Error", "Mobile processing failed", ex.ToString());
                throw;
            }
        }

        private async Task LogAsync(string level, string message, string? exception = "No Exception",
            string? donatie_ID = null)
        {
            var log = new LogEntry
            {
                datumtijd = DateTime.UtcNow,
                level = level,
                bericht = message,
                exception = exception,  
                donatie_ID = donatie_ID
            };

            _context.Logs.Add(log);
            await _context.SaveChangesAsync();
        }
         
        [HttpGet("status")]
        public IActionResult CheckStatus()
        {
            return Ok("Systeem werkt");
        }
    }
}
